/**************************************************************
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *************************************************************/

package com.sun.star.sdbcx.comp.postgresql.sdbcx;

import com.sun.star.container.XNameAccess;
import com.sun.star.lang.DisposedException;
import com.sun.star.lang.XServiceInfo;
import com.sun.star.lib.uno.helper.ComponentBase;
import com.sun.star.sdbc.SQLException;
import com.sun.star.sdbc.XDatabaseMetaData;
import com.sun.star.sdbc.XRow;
import com.sun.star.sdbcx.XGroupsSupplier;
import com.sun.star.sdbcx.XTablesSupplier;
import com.sun.star.sdbcx.XUsersSupplier;
import com.sun.star.sdbcx.XViewsSupplier;
import com.sun.star.sdbcx.comp.postgresql.util.ComposeRule;
import com.sun.star.sdbcx.comp.postgresql.util.DbTools;

public abstract class OCatalog extends ComponentBase
        implements XTablesSupplier, XViewsSupplier, XUsersSupplier, XGroupsSupplier, XServiceInfo {

    private static final String[] services = {
            "com.sun.star.sdbcx.DatabaseDefinition"
    };

    protected final Object lock = new Object();
    // Deleted on destruction, weakly held by caller:
    protected OContainer tables;
    protected OContainer views;
    protected OContainer groups;
    protected OContainer users;
    protected XDatabaseMetaData metadata;

    public OCatalog(XDatabaseMetaData metadata) {
        this.metadata = metadata;
    }

    private void checkDisposed() throws DisposedException {
        if (bInDispose || bDisposed) {
            throw new DisposedException();
        }
    }

    @Override
    protected void postDisposing() {
        synchronized (lock) {
            if (tables != null) {
                tables.dispose();
            }
            if (views != null) {
                views.dispose();
            }
            if (groups != null) {
                groups.dispose();
            }
            if (users != null) {
                users.dispose();
            }
        }
    }

    // XServiceInfo

    @Override
    public String getImplementationName() {
        return "com.sun.star.comp.connectivity.OCatalog";
    }

    @Override
    public String[] getSupportedServiceNames() {
        return services.clone();
    }

    @Override
    public boolean supportsService(String serviceName) {
        for (String service : getSupportedServiceNames()) {
            if (service.equals(serviceName)) {
                return true;
            }
        }
        return false;
    }

    // X(Tables/Views/Groups/Users)Supplier

    @Override
    public XNameAccess getTables() {
        checkDisposed();
        synchronized (lock) {
            if (tables == null) {
                tables = refreshTables();
            }
            return tables;
        }
    }

    @Override
    public XNameAccess getViews() {
        checkDisposed();
        synchronized (lock) {
            if (views == null) {
                views = refreshViews();
            }
            return views;
        }
    }

    @Override
    public XNameAccess getGroups() {
        checkDisposed();
        synchronized (lock) {
            if (groups == null) {
                groups = refreshGroups();
            }
            return groups;
        }
    }

    @Override
    public XNameAccess getUsers() {
        checkDisposed();
        synchronized (lock) {
            if (users == null) {
                users = refreshUsers();
            }
            return users;
        }
    }

    protected String buildName(XRow row) throws SQLException {
        String catalog = row.getString(1);
        if (row.wasNull()) {
            catalog = "";
        }
        String schema = row.getString(2);
        if (row.wasNull()) {
            schema = null;
        }
        String table = row.getString(3);
        if (row.wasNull()) {
            table = "";
        }
        return DbTools.composeTableName(metadata, catalog, schema, table, false, ComposeRule.InDataManipulation);
    }

    public abstract OContainer refreshTables();
    public abstract OContainer refreshViews();
    public abstract OContainer refreshGroups();
    public abstract OContainer refreshUsers();
}
