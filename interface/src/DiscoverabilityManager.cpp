//
//  DiscoverabilityManager.cpp
//  interface/src
//
//  Created by Stephen Birarda on 2015-03-09.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QJsonDocument>

#include <AccountManager.h>
#include <AddressManager.h>
#include <DomainHandler.h>
#include <NodeList.h>
#include <UUID.h>

#include "DiscoverabilityManager.h"
#include "Menu.h"

const Discoverability::Mode DEFAULT_DISCOVERABILITY_MODE = Discoverability::All;

DiscoverabilityManager::DiscoverabilityManager() :
    _mode("discoverabilityMode", DEFAULT_DISCOVERABILITY_MODE)
{
    qRegisterMetaType<Discoverability::Mode>("Discoverability::Mode");
}

const QString API_USER_LOCATION_PATH = "/api/v1/user/location";

void DiscoverabilityManager::updateLocation() {
    AccountManager& accountManager = AccountManager::getInstance();
    
    if (_mode.get() != Discoverability::None) {
        auto addressManager = DependencyManager::get<AddressManager>();
        DomainHandler& domainHandler = DependencyManager::get<NodeList>()->getDomainHandler();
        
        if (accountManager.isLoggedIn() && domainHandler.isConnected()
            && (!addressManager->getRootPlaceID().isNull() || !domainHandler.getUUID().isNull())) {
            
            // construct a QJsonObject given the user's current address information
            QJsonObject rootObject;
            
            QJsonObject locationObject;
            
            QString pathString = addressManager->currentPath();
            
            const QString LOCATION_KEY_IN_ROOT = "location";
            
            const QString PATH_KEY_IN_LOCATION = "path";
            locationObject.insert(PATH_KEY_IN_LOCATION, pathString);
            
            if (!addressManager->getRootPlaceID().isNull()) {
                const QString PLACE_ID_KEY_IN_LOCATION = "place_id";
                locationObject.insert(PLACE_ID_KEY_IN_LOCATION,
                                      uuidStringWithoutCurlyBraces(addressManager->getRootPlaceID()));
                
            } else {
                const QString DOMAIN_ID_KEY_IN_LOCATION = "domain_id";
                locationObject.insert(DOMAIN_ID_KEY_IN_LOCATION,
                                      uuidStringWithoutCurlyBraces(domainHandler.getUUID()));
            }
            
            const QString FRIENDS_ONLY_KEY_IN_LOCATION = "friends_only";
            locationObject.insert(FRIENDS_ONLY_KEY_IN_LOCATION, (_mode.get() == Discoverability::Friends));
            
            rootObject.insert(LOCATION_KEY_IN_ROOT, locationObject);
            
            accountManager.sendRequest(API_USER_LOCATION_PATH, AccountManagerAuth::Required,
                                       QNetworkAccessManager::PutOperation,
                                       JSONCallbackParameters(), QJsonDocument(rootObject).toJson());
        }
    } else {
        // we still send a heartbeat to the metaverse server for stats collection
        const QString API_USER_HEARTBEAT_PATH = "/api/v1/user/heartbeat";
        accountManager.sendRequest(API_USER_HEARTBEAT_PATH, AccountManagerAuth::Required, QNetworkAccessManager::PutOperation);
    }
}

void DiscoverabilityManager::removeLocation() {
    AccountManager& accountManager = AccountManager::getInstance();
    accountManager.sendRequest(API_USER_LOCATION_PATH, AccountManagerAuth::Required, QNetworkAccessManager::DeleteOperation);
}

void DiscoverabilityManager::setDiscoverabilityMode(Discoverability::Mode discoverabilityMode) {
    if (static_cast<Discoverability::Mode>(_mode.get()) != discoverabilityMode) {
        
        // update the setting to the new value
        _mode.set(static_cast<int>(discoverabilityMode));
        
        if (static_cast<int>(_mode.get()) == Discoverability::None) {
            // if we just got set to no discoverability, make sure that we delete our location in DB
            removeLocation();
        }

        emit discoverabilityModeChanged(discoverabilityMode);
    }
}

void DiscoverabilityManager::setVisibility() {
    Menu* menu = Menu::getInstance();

    if (menu->isOptionChecked(MenuOption::VisibleToEveryone)) {
        this->setDiscoverabilityMode(Discoverability::All);
    } else if (menu->isOptionChecked(MenuOption::VisibleToFriends)) {
        this->setDiscoverabilityMode(Discoverability::Friends);
    } else if (menu->isOptionChecked(MenuOption::VisibleToNoOne)) {
        this->setDiscoverabilityMode(Discoverability::None);
    } else {
        qDebug() << "ERROR DiscoverabilityManager::setVisibility() called with unrecognized value.";
    }
}

void DiscoverabilityManager::visibilityChanged(Discoverability::Mode discoverabilityMode) {
    Menu* menu = Menu::getInstance();

    if (discoverabilityMode == Discoverability::All) {
        menu->setIsOptionChecked(MenuOption::VisibleToEveryone, true);
    } else if (discoverabilityMode == Discoverability::Friends) {
        menu->setIsOptionChecked(MenuOption::VisibleToFriends, true);
    } else if (discoverabilityMode == Discoverability::None) {
        menu->setIsOptionChecked(MenuOption::VisibleToNoOne, true);
    } else {
        qDebug() << "ERROR DiscoverabilityManager::visibilityChanged() called with unrecognized value.";
    }
}
