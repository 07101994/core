/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <gtk/gtk.h>

#include <LibreOfficeKit/LibreOfficeKitGtk.h>
#include <LibreOfficeKit/LibreOfficeKitEnums.h>

#include <gtv-application-window.hxx>
#include <gtv-main-toolbar.hxx>
#include <gtv-helpers.hxx>
#include <gtv-lokdocview-signal-handlers.hxx>

#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

struct GtvApplicationWindowPrivate
{
    GtkWidget* container;
    GtkWidget* toolbarcontainer;

    gboolean toolbarBroadcast;

    // Rendering args; options with which lokdocview was rendered in this window
    GtvRenderingArgs* m_pRenderingArgs;
};

G_DEFINE_TYPE_WITH_PRIVATE(GtvApplicationWindow, gtv_application_window, GTK_TYPE_APPLICATION_WINDOW);

static GtvApplicationWindowPrivate*
getPrivate(GtvApplicationWindow* win)
{
    return static_cast<GtvApplicationWindowPrivate*>(gtv_application_window_get_instance_private(win));
}

static void
gtv_application_window_init(GtvApplicationWindow* win)
{
    GtkBuilder* builder = gtk_builder_new_from_file("gtv.ui");
    GtvApplicationWindowPrivate* priv = getPrivate(win);

    // This is the parent GtkBox holding everything
    priv->container = GTK_WIDGET(gtk_builder_get_object(builder, "container"));
    // Toolbar container
    priv->toolbarcontainer = gtv_main_toolbar_new();

    // Attach to the toolbar to main window
    gtk_box_pack_start(GTK_BOX(priv->container), priv->toolbarcontainer, false, false, false);
    gtk_box_reorder_child(GTK_BOX(priv->container), priv->toolbarcontainer, 0);

    // scrolled window containing the main drawing area
    win->scrolledwindow = GTK_WIDGET(gtk_builder_get_object(builder, "scrolledwindow"));

    // statusbar
    win->statusbar = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
    win->redlinelabel = GTK_WIDGET(gtk_builder_get_object(builder, "redlinelabel"));
    win->zoomlabel = GTK_WIDGET(gtk_builder_get_object(builder, "zoomlabel"));

    win->findtoolbar = GTK_WIDGET(gtk_builder_get_object(builder, "findtoolbar"));
    win->findbarlabel = GTK_WIDGET(gtk_builder_get_object(builder, "findbar_label"));
    priv->toolbarBroadcast = false;

    gtk_container_add(GTK_CONTAINER(win), priv->container);

    g_object_unref(builder);

    priv->m_pRenderingArgs = new GtvRenderingArgs();
}

static void
gtv_application_window_dispose(GObject* object)
{
    GtvApplicationWindowPrivate* priv = getPrivate(GTV_APPLICATION_WINDOW(object));

    delete priv->m_pRenderingArgs;
    priv->m_pRenderingArgs = nullptr;

    G_OBJECT_CLASS (gtv_application_window_parent_class)->dispose (object);
}

static void
gtv_application_window_class_init(GtvApplicationWindowClass* klass)
{
    // TODO: Use templates to bind objects maybe ?
    // But that requires compiling the .ui file into C source requiring
    // glib-compile-resources (another dependency) as I can't find any gtk
    // method to set the template from the .ui file directly; can only be set
    // from gresource
    G_OBJECT_CLASS(klass)->dispose = gtv_application_window_dispose;
}

static void
gtv_application_open_document_callback(GObject* source_object, GAsyncResult* res, gpointer /*userdata*/)
{
    LOKDocView* pDocView = LOK_DOC_VIEW (source_object);
    GError* error = nullptr;
    if (!lok_doc_view_open_document_finish(pDocView, res, &error))
    {
        GtkDialogFlags eFlags = GTK_DIALOG_DESTROY_WITH_PARENT;
        GtkWidget* pDialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(pDocView))),
                                                    eFlags,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_CLOSE,
                                                    "Error occurred while opening the document: '%s'",
                                                    error->message);
        gtk_dialog_run(GTK_DIALOG(pDialog));
        gtk_widget_destroy(pDialog);

        g_error_free(error);
        gtk_widget_destroy(GTK_WIDGET(pDocView));
        gtk_main_quit();
        return;
    }

    lok_doc_view_set_edit(pDocView, true);
}

/// Get the visible area of the scrolled window
void gtv_application_window_get_visible_area(GtvApplicationWindow* pWindow, GdkRectangle* pArea)
{
    GtkAdjustment* pHAdjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(pWindow->scrolledwindow));
    GtkAdjustment* pVAdjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(pWindow->scrolledwindow));

    pArea->x      = lok_doc_view_pixel_to_twip(LOK_DOC_VIEW(pWindow->lokdocview),
                                               gtk_adjustment_get_value(pHAdjustment));
    pArea->y      = lok_doc_view_pixel_to_twip(LOK_DOC_VIEW(pWindow->lokdocview),
                                               gtk_adjustment_get_value(pVAdjustment));
    pArea->width  = lok_doc_view_pixel_to_twip(LOK_DOC_VIEW(pWindow->lokdocview),
                                               gtk_adjustment_get_page_size(pHAdjustment));
    pArea->height = lok_doc_view_pixel_to_twip(LOK_DOC_VIEW(pWindow->lokdocview),
                                               gtk_adjustment_get_page_size(pVAdjustment));
}

void gtv_application_window_toggle_findbar(GtvApplicationWindow* window)
{
    if (gtk_widget_get_visible(window->findtoolbar))
    {
        gtk_widget_hide(window->findtoolbar);
    }
    else
    {
        gtk_widget_show_all(window->findtoolbar);
        gtk_widget_grab_focus(window->findtoolbar);
    }
}

GtkToolItem* gtv_application_window_find_tool_by_unocommand(GtvApplicationWindow* window, const std::string& unoCmd)
{
    GtvApplicationWindowPrivate* priv = getPrivate(window);
    GtkToolItem* result = nullptr;

    // Find in the first toolbar
    GtkContainer* pToolbar1 = gtv_main_toolbar_get_first_toolbar(GTV_MAIN_TOOLBAR(priv->toolbarcontainer));
    GList* pList = gtk_container_get_children(pToolbar1);
    for (GList* l = pList; l != nullptr; l = l->next)
    {
        if (GTK_IS_TOOL_BUTTON(l->data))
        {
            GtkToolButton* pButton = GTK_TOOL_BUTTON(l->data);
            const gchar* pLabel = gtk_tool_button_get_label(pButton);
            if (g_strcmp0(unoCmd.c_str(), pLabel) == 0)
            {
                result = GTK_TOOL_ITEM(pButton);
            }
        }
    }

    // Look in second toolbar if not found
    GtkContainer* pToolbar2 = gtv_main_toolbar_get_second_toolbar(GTV_MAIN_TOOLBAR(priv->toolbarcontainer));
    pList = gtk_container_get_children(pToolbar2);
    for (GList* l = pList; result == nullptr && l != nullptr; l = l->next)
    {
        if (GTK_IS_TOOL_BUTTON(l->data))
        {
            GtkToolButton* pButton = GTK_TOOL_BUTTON(l->data);
            const gchar* pLabel = gtk_tool_button_get_label(pButton);
            if (g_strcmp0(unoCmd.c_str(), pLabel) == 0)
            {
                result = GTK_TOOL_ITEM(pButton);
            }
        }
    }

    return result;
}

static const std::string
createRenderingArgsJSON(const GtvRenderingArgs* pRenderingArgs)
{
    boost::property_tree::ptree aTree;
    if (pRenderingArgs->m_bHidePageShadow)
    {
        aTree.put(boost::property_tree::ptree::path_type(".uno:ShowBorderShadow/type", '/'), "boolean");
        aTree.put(boost::property_tree::ptree::path_type(".uno:ShowBorderShadow/value", '/'), false);
    }
    if (pRenderingArgs->m_bHideWhiteSpace)
    {
        aTree.put(boost::property_tree::ptree::path_type(".uno:HideWhitespace/type", '/'), "boolean");
        aTree.put(boost::property_tree::ptree::path_type(".uno:HideWhitespace/value", '/'), true);
    }
    aTree.put(boost::property_tree::ptree::path_type(".uno:Author/type", '/'), "string");
    aTree.put(boost::property_tree::ptree::path_type(".uno:Author/value", '/'), getNextAuthor());
    std::stringstream aStream;
    boost::property_tree::write_json(aStream, aTree);
    return aStream.str();
}

static void setupDocView(LOKDocView* pDocView)
{
#if GLIB_CHECK_VERSION(2,40,0)
    g_assert_nonnull(pDocView);
#endif
    g_signal_connect(pDocView, "edit-changed", G_CALLBACK(lokdocview_signalEdit), nullptr);
    g_signal_connect(pDocView, "command-changed", G_CALLBACK(lokdocview_signalCommand), nullptr);
    g_signal_connect(pDocView, "command-result", G_CALLBACK(lokdocview_signalCommandResult), nullptr);
/*    g_signal_connect(pDocView, "search-not-found", G_CALLBACK(lokdocview_signalSearch), nullptr);
    g_signal_connect(pDocView, "search-result-count", G_CALLBACK(lokdocview_signalSearchResultCount), nullptr);
    g_signal_connect(pDocView, "part-changed", G_CALLBACK(lokdocview_signalPart), nullptr);
    g_signal_connect(pDocView, "hyperlink-clicked", G_CALLBACK(lokdocview_signalHyperlink), nullptr);
    g_signal_connect(pDocView, "cursor-changed", G_CALLBACK(lokdocview_cursorChanged), nullptr);
    g_signal_connect(pDocView, "address-changed", G_CALLBACK(lokdocview_addressChanged), nullptr);
    g_signal_connect(pDocView, "formula-changed", G_CALLBACK(lokdocview_formulaChanged), nullptr);
    g_signal_connect(pDocView, "password-required", G_CALLBACK(lokdocview_passwordRequired), nullptr);
    g_signal_connect(pDocView, "comment", G_CALLBACK(lokdocview_commentCallback), nullptr);
*/
}

void
gtv_application_window_create_view_from_window(GtvApplicationWindow* window)
{
    GtvApplicationWindowPrivate* priv = getPrivate(window);
    GApplication* app = g_application_get_default();

    GtvApplicationWindow* newWindow = GTV_APPLICATION_WINDOW(gtv_application_window_new(GTK_APPLICATION(app)));
    const std::string aArguments = createRenderingArgsJSON(priv->m_pRenderingArgs);
    newWindow->lokdocview = lok_doc_view_new_from_widget(LOK_DOC_VIEW(window->lokdocview), aArguments.c_str());
    setupDocView(LOK_DOC_VIEW(newWindow->lokdocview));

    gboolean bTiledAnnotations;
    g_object_get(G_OBJECT(window->lokdocview), "tiled-annotations", &bTiledAnnotations, nullptr);
    gtk_container_add(GTK_CONTAINER(newWindow->scrolledwindow), newWindow->lokdocview);
    gtk_widget_show_all(newWindow->scrolledwindow);
    gtk_window_present(GTK_WINDOW(newWindow));
}

void
gtv_application_window_load_document(GtvApplicationWindow* window,
                                     const GtvRenderingArgs* aArgs,
                                     const std::string& aDocPath)
{
    GtvApplicationWindowPrivate* priv = getPrivate(window);
    // keep a copy of it; we need to use these for creating new views later
    *(priv->m_pRenderingArgs) = *aArgs;

    // setup lokdocview
    window->lokdocview = lok_doc_view_new_from_user_profile(priv->m_pRenderingArgs->m_aLoPath.c_str(),
                                                            priv->m_pRenderingArgs->m_aUserProfile.empty() ? nullptr : priv->m_pRenderingArgs->m_aUserProfile.c_str(),
                                                          nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(window->scrolledwindow), window->lokdocview);
    g_object_set(G_OBJECT(window->lokdocview),
                 "doc-password", TRUE,
                 "doc-password-to-modify", TRUE,
                 "tiled-annotations", priv->m_pRenderingArgs->m_bEnableTiledAnnotations,
                 nullptr);
    setupDocView(LOK_DOC_VIEW(window->lokdocview));

    // Create argument JSON
    const std::string aArguments = createRenderingArgsJSON(priv->m_pRenderingArgs);
    lok_doc_view_open_document(LOK_DOC_VIEW(window->lokdocview), aDocPath.c_str(),
                               aArguments.c_str(), nullptr,
                               gtv_application_open_document_callback, window->lokdocview);

    gtk_widget_show_all(GTK_WIDGET(window->scrolledwindow));
}

GtvMainToolbar*
gtv_application_window_get_main_toolbar(GtvApplicationWindow* window)
{
    GtvApplicationWindowPrivate* priv = getPrivate(window);
    return GTV_MAIN_TOOLBAR(priv->toolbarcontainer);
}

void
gtv_application_window_set_toolbar_broadcast(GtvApplicationWindow* window, bool broadcast)
{
    GtvApplicationWindowPrivate* priv = getPrivate(window);
    priv->toolbarBroadcast = broadcast;
}

gboolean
gtv_application_window_get_toolbar_broadcast(GtvApplicationWindow* window)
{
    GtvApplicationWindowPrivate* priv = getPrivate(window);
    return priv->toolbarBroadcast;
}

GtvApplicationWindow*
gtv_application_window_new(GtkApplication* app)
{
    g_return_val_if_fail(GTK_IS_APPLICATION(app), nullptr);

    return GTV_APPLICATION_WINDOW(g_object_new(GTV_APPLICATION_WINDOW_TYPE,
                                               "application", app,
                                               "width-request", 1024,
                                               "height-request", 768,
                                               "title", "LibreOffice GtkTiledViewer",
                                               "window-position", GTK_WIN_POS_CENTER,
                                               "show-menubar", false,
                                               nullptr));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
