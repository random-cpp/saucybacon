/**
 * This file is part of SaucyBacon.
 *
 * Copyright 2013-2015 (C) Giulio Collura <random.cpp@gmail.com>
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

import QtQuick 2.3
import QtQuick.Layouts 1.0
import Ubuntu.Components 1.1
import Ubuntu.Components.ListItems 1.0 as ListItem
import Ubuntu.Layouts 1.0
import SaucyBacon 1.0

import "../components"

Page {
    id: page
    title: i18n.tr("Search")

    head.actions: [
        Action {
            id: searchTopRatedAction
            text: i18n.tr("Top Rated")
            description: i18n.tr("Search top rated recipes")
            iconName: "favorite-selected"
            keywords: "search;top;rated;recipe"
            onTriggered: { searchOnline(""); }
        }
    ]

    head.contents: TextField {
        id: searchField
        objectName: "searchField"

        width: parent.width - units.gu(2)
        placeholderText: i18n.tr("Search online for a recipe...")

        onAccepted: searchOnline(searchField.text)
        onTextChanged: searchLocally(searchField.text)

        Behavior on width { UbuntuNumberAnimation { } }
    }

    opacity: visible ? 1 : 0
    Behavior on opacity {
        UbuntuNumberAnimation {
            duration: UbuntuAnimation.SlowDuration
        }
    }

    LoadingIndicator {
        id: loadingIndicator
        text: i18n.tr("Searching...")
        isShown: search.loading
    }

    Layouts {
        id: layouts
        anchors.fill: parent

        layouts: [
            ConditionalLayout {
                name: "tabletLayout"
                when: wideAspect

                RowLayout {
                    anchors.fill: parent

                    Sidebar {
                        id: sidebar
                        mode: "left"
                        anchors {
                            top: parent.top
                            bottom: parent.bottom
                        }

                        Column {
                            anchors {
                                left: parent.left
                                right: parent.right
                            }

                            ListItem.Header {
                                text: i18n.tr("Search history")
                            }

                            Repeater {
                                model: database.searches

                                ListItem.Standard {
                                    text: modelData.name
                                    onClicked: {
                                        searchField.text = modelData.name;
                                        searchOnline(modelData.name);
                                    }
                                }
                            }
                        }
                    }

                    ItemLayout {
                        item: "searchColumn"
                        anchors {
                            top: parent.top
                            bottom: parent.bottom
                            topMargin: units.gu(2)
                        }
                        Layout.fillWidth: true
                    }
                }
            }
        ]

        ColumnLayout {
            id: searchColumn
            Layouts.item: "searchColumn"
            objectName: "searchColumn"

            anchors {
                fill: parent
                topMargin: units.gu(2)
            }
            spacing: units.gu(2)

            Label {
                id: creditLabel
                anchors {
                    right: parent.right
                    rightMargin: units.gu(4)
                }
                text: i18n.tr("Powered by Food2Fork.com")
                fontSize: "small"
            }

            RefreshableListView {
                id: resultList
                objectName: "resultList"
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Layout.fillHeight: true
                clip: true
                cacheBuffer: units.gu(8) * 20

                model: search

                /* A delegate will be created for each Document retrieved from the Database */
                delegate: ListItem.Subtitled {
                    progression: true
                    iconSource: contents.image_url
                    text: contents.title
                    subText: contents.publisher_url
                    height: units.gu(8)
                    onClicked: {
                        database.getRecipeOnline(contents.recipe_id, contents.source_url, contents.publisher_url, contents.image_url);
                        pageStack.push(Qt.resolvedUrl("RecipePage.qml"));
                    }
                }

                onPulledUp: {
                    console.log("Load more results")
                    oldContentY = contentY;
                    search.loadMore();
                }

                property double oldContentY;

                UbuntuNumberAnimation on contentY {
                    id: scrollingAnimation
                }

                Scrollbar {
                    flickableItem: resultList
                }
            }
        }
    }

    RecipeSearch {
        id: search
        onLoadingError: {
            loadingIndicator.text = i18n.tr("Loading error, please try again");
        }
        onLoadingCompleted: {
            if (search.page > 1) {
                resultList.contentY = resultList.oldContentY;
                scrollingAnimation.to = resultList.contentY + units.gu(3);
                scrollingAnimation.start();
            }
        }
    }

    function searchOnline(querystr) {
        // Since the number of the api calls is limited,
        // it's better to keep the online search a real request by the user
        // TODO: have money to buy an unlimited API

        console.log("Perfoming remote search...");
        loadingIndicator.text = i18n.tr("Searching...");
        search.query = querystr;

        if (querystr.length > 0) {
            database.addSearch(querystr);
        }
    }

    function searchLocally(querystr) {
        // Perform a local search on our personal db
        // this function can be called everytime the user write text in the entry

        //searchQuery.query = [ {"title": querystr + "*" , "name": querystr + "*" }]
    }
}
