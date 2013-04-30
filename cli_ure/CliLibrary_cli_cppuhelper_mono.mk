# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

include $(SRCDIR)/cli_ure/version/version.txt

$(eval $(call gb_CliLibrary_CliLibrary,cli_cppuhelper))

$(eval $(call gb_CliLibrary_set_configfile,cli_cppuhelper,cli_ure/source/native/cli_cppuhelper_config))

$(eval $(call gb_CliLibrary_set_keyfile,cli_cppuhelper,$(SRCDIR)/cli_ure/source/cliuno.snk))

$(eval $(call gb_CliLibrary_set_policy,cli_cppuhelper,$(CLI_CPPUHELPER_POLICY_ASSEMBLY),$(CLI_CPPUHELPER_POLICY_VERSION)))

$(eval $(call gb_CliLibrary_add_csfiles,cli_cppuhelper,\
    cli_ure/source/bootstrap/managed_bootstrap \
))

$(eval $(call gb_CliLibrary_add_generated_csfiles,cli_cppuhelper,\
	CustomTarget/cli_ure/source/bootstrap/assembly \
))

$(eval $(call gb_CliLibrary_use_assemblies,cli_cppuhelper,\
	cli_uretypes \
))

# vim: set noet sw=4 ts=4:
