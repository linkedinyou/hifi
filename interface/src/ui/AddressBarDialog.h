//
//  AddressBarDialog.h
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_AddressBarDialog_h
#define hifi_AddressBarDialog_h

#include <OffscreenQmlDialog.h>

class AddressBarDialog : public OffscreenQmlDialog
{
    Q_OBJECT
    HIFI_QML_DECL

public:
    AddressBarDialog(QQuickItem* parent = nullptr);

protected:
    void displayAddressOfflineMessage();
    void displayAddressNotFoundMessage();
    void hide();

    Q_INVOKABLE void loadAddress(const QString& address);
};

#endif
