/****************************************************************************
**
** Copyright (C) 2016 Kurt Pattyn <pattyn.kurt@gmail.com>.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebSockets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "chatserver.h"

#include <QtCore>
#include <QtWebSockets>

#include <cstdio>
#include <QTimer>
#include <QElapsedTimer>
using namespace std;

QT_USE_NAMESPACE

static QString getIdentifier(QWebSocket* peer) {
	return QStringLiteral("%1:%2").arg(peer->peerAddress().toString(),
	                                   QString::number(peer->peerPort()));
}

//! [constructor]
ChatServer::ChatServer(quint16 port, QObject* parent) : QObject(parent),
                                                        m_pWebSocketServer(new QWebSocketServer(QStringLiteral("Chat Server"),
                                                                                                QWebSocketServer::NonSecureMode,
                                                                                                this)) {
	if (m_pWebSocketServer->listen(QHostAddress::Any, port)) {
		QTextStream(stdout) << "Chat Server listening on port " << port << '\n';
		connect(m_pWebSocketServer, &QWebSocketServer::newConnection,this, &ChatServer::onNewConnection);
	}
	QTimer* timer = new QTimer();
	timer->setInterval(50);
	timer->start();
	connect(timer,&QTimer::timeout,this, &ChatServer::broadCast1);
	elapsedTimer.start();

}

ChatServer::~ChatServer() {
	m_pWebSocketServer->close();
}
//! [constructor]

//! [onNewConnection]
void ChatServer::onNewConnection() {
	auto pSocket = m_pWebSocketServer->nextPendingConnection();
	QTextStream(stdout) << getIdentifier(pSocket) << " connected!\n";
	pSocket->setParent(this);

	connect(pSocket, &QWebSocket::textMessageReceived,
	        this, &ChatServer::processMessage);
	connect(pSocket, &QWebSocket::disconnected,
	        this, &ChatServer::socketDisconnected);

	m_clients << pSocket;
}
//! [onNewConnection]

//! [processMessage]
void ChatServer::processMessage(const QString& message) {
	QWebSocket* pSender = qobject_cast<QWebSocket*>(sender());
	for (QWebSocket* pClient : qAsConst(m_clients)) {
		if (pClient != pSender) //don't echo message back to sender
			pClient->sendTextMessage(message);
	}
}
//! [processMessage]

//! [socketDisconnected]
void ChatServer::socketDisconnected() {
	QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
	QTextStream(stdout) << getIdentifier(pClient) << " disconnected!\n";
	if (pClient) {
		m_clients.removeAll(pClient);
		pClient->deleteLater();
	}
}

void ChatServer::broadCast1() {
	static const QString skel = R"EOD(
{
"action": "toggleButton",
"payload" : %1
}
)EOD";
	for (QWebSocket* pClient : qAsConst(m_clients)) {
		pClient->sendTextMessage(skel.arg(elapsedTimer.nsecsElapsed() / 1000000));
	}
}
//! [socketDisconnected]
