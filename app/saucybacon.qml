/**
 * This file is part of SaucyBacon.
 *
 * Copyright 2013-2014 (C) Giulio Collura <random.cpp@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

import QtQuick 2.0
import Ubuntu.Components 1.1
import U1db 1.0 as U1db
import SaucyBacon 1.0

import "ui"
import "components"
import "backend"

import "backend/prototypes.js" as Prototypes

MainView {
    objectName: "mainView"
    id: mainView

    // NOTE: applicationName needs to match the .desktop filename
    applicationName: "com.ubuntu.developer.gcollura.saucybacon"

    automaticOrientation: true
    anchorToKeyboard: true
    useDeprecatedToolbar: false
    property bool wideAspect: width > units.gu(80)

    width: units.gu(135)
    height: units.gu(85)

    // Thanks Lucas Di Benedetto
    headerColor: "#6d0a0a"
    backgroundColor: colors.darkRed
    footerColor: "#370517"

    // Global actions

    Action {
        id: newRecipeAction
        text: i18n.tr("New")
        description: i18n.tr("Create a new recipe")
        iconName: "add"
        keywords: "new;recipe"
        onTriggered: {
            recipe.newRecipe();
            pageStack.push(Qt.resolvedUrl("ui/EditPage.qml"), { title: i18n.tr("New Recipe") });
        }
    }

    Action {
        id: editRecipeAction
        text: i18n.tr("Edit")
        description: i18n.tr("Edit the current recipe")
        iconName: "edit"
        keywords: "edit;recipe"
        onTriggered: pageStack.push(Qt.resolvedUrl("ui/EditPage.qml"), { title: i18n.tr("Edit Recipe") });
    }

    Action {
        id: searchAction
        text: i18n.tr("Search")
        description: i18n.tr("Search for a new recipe on the internet")
        iconName: "search"
        keywords: "search;new;recipe"
        onTriggered: { pageStack.push(Qt.resolvedUrl("ui/SearchPage.qml"))}
    }

    Action {
        id: aboutAction
        text: i18n.tr("About")
        description: i18n.tr("About this application...")
        iconName: "help"
        keywords: "about;saucybacon"
        onTriggered: { pageStack.push(Qt.resolvedUrl("ui/AboutPage.qml"))}
    }

    actions: [ newRecipeAction, searchAction ]

    PageStack {
        objectName: "pageStack"
        id: pageStack

        Component.onCompleted: {
            push(homePage);
        }

        HomePage {
            objectName: "homePage"
            id: homePage
            tools: ToolbarItems {
                ToolbarButton {
                    action: newRecipeAction
                }
                ToolbarButton {
                    action: searchAction
                }
            }
        }
    }

    Component.onCompleted: {
        loadSettings();
    }

    Component.onDestruction: {
        saveSettings();
    }

    Colors {
        id: colors
    }

    // SaucyBacon Utils library
    Utils {
        id: utils
        property string version: "0.2.0"
    }

    /* Recipe Database */
    Database {
        id: saucybacondb
        path: utils.path(Utils.SettingsLocation, "sb-recipes.db")
    }

    U1db.Index {
        id: recipes
        database: saucybacondb
        name: 'recipes'
        expression: [ 'name', 'category', 'restriction', 'favorite', 'difficulty', 
            'photos', 'preptime', 'cooktime' ]
    }

    U1db.Query {
        id: recipesdb
        index: recipes
        query: "*"

        property int count: results.length
    }

    /* Base recipe document - just for reference
    U1db.Document {
        database: db
        create: false
        defaults: { "name": "", "category": "", "difficulty": 1, "restriction": 0,
            "preptime": "0", "cooktime": "0", "totaltime": "0", "ingredients": [ ],
            "directions": "", "servings": 4, "photos" : [ ], "favorite": false }
    } */

    property Recipe r: Recipe { id: recipe }

    /* Recipe addons */
    property var difficulties: [ i18n.tr("No difficulty"), i18n.tr("Easy"), i18n.tr("Medium"), i18n.tr("Hard") ] // FIXME: Strange naming
    property var categories: [ ]
    property var restrictions: [ i18n.tr("Non-veg"), i18n.tr("Vegetarian"), i18n.tr("Vegan") ]
    property var searches: [ ]

    function loadSettings() {

        if (!utils.get("firstLoad")) {
            console.log("Initializing settings and database for the first time.");
            categories = [ i18n.tr("Uncategorized") ];

            utils.set("firstLoad", 1);
            utils.set("version", utils.version);
        } else {
            console.log("Reloading last saved options.")
            // Restore previous size
            height = utils.get("windowSize").height;
            width = utils.get("windowSize").width;
            categories = utils.get("categories");
            searches = utils.get("searches");

            if (utils.get("version") != utils.version)
                updateDB(utils.get("version"));
        }

        // Component.onDestruction isn't called on the phone
        categoriesChanged.connect(saveSettings);
        searchesChanged.connect(saveSettings);
        saveSettings();
    }

    function saveSettings() {
        utils.set("windowSize", { "height": height, "width": width });
        utils.set("categories", categories);
        utils.set("searches", searches);
        utils.set("version", utils.version);
        utils.save();
    }

    // Helper functions
    function icon(name, local) {
        return Qt.resolvedUrl("graphics/" + name + ".png")
    }

    function truncate(name, width, unit) {
        unit = typeof unit === "undefined" ? units.gu(2) : unit
        if (typeof name === "undefined") return "";
        if (name.length > width / unit) {
            name = name.substring(0, width / (unit + units.gu(0.2)));
            name += "...";
        }
        return name;
    }

    function updateDB(oldVersion) {
        oldVersion = typeof oldVersion === "undefined" ? "0.1.0" : oldVersion
        if (oldVersion.startsWith("0.1")) {
            console.log("Migrating from " + oldVersion + " to " + utils.version)
            var docs = recipesdb.listDocs();
            for (var i = 0; i < docs.length; i++) {
                var contents = recipesdb.getDoc(docs[i])
                contents["preptime"] = parseInt(contents["preptime"]);
                contents["cooktime"] = parseInt(contents["cooktime"]);
                recipesdb.putDoc(contents, docs[i]);
            }
        }
    }
}
