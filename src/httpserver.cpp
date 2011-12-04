/*  This file is part of the Tufão project
    Copyright (C) 2011 Vinícius dos Santos Oliveira <vini.ipsmaker@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any
    later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "httpserver.h"
#include "priv/httpserver.h"
#include "httpserverrequest.h"
#include <QTcpSocket>

namespace Tufao {

HttpServer::HttpServer(QObject *parent) :
    QObject(parent),
    priv(new Priv::HttpServer)
{
    connect(&priv->tcpServer, SIGNAL(newConnection(int)),
            this, SLOT(onNewConnection(int)));
}

HttpServer::~HttpServer()
{
    delete priv;
}

bool HttpServer::listen(const QHostAddress &address, quint16 port)
{
    return priv->tcpServer.listen(address, port);
}

void HttpServer::close()
{
    priv->tcpServer.close();
}

void HttpServer::incomingConnection(int socketDescriptor)
{
    QTcpSocket *socket = new QTcpSocket;

    if (!socket->setSocketDescriptor(socketDescriptor)) {
        delete socket;
        return;
    }

    handleConnection(socket);
}

void HttpServer::handleConnection(QAbstractSocket *socket)
{
    socket->setParent(this);
    HttpServerRequest *handle = new HttpServerRequest(socket, this);

    connect(handle, SIGNAL(ready(Tufao::HttpServerResponse::Options)),
            this, SLOT(onRequestReady(Tufao::HttpServerResponse::Options)));
    connect(socket, SIGNAL(disconnected()), handle, SLOT(deleteLater()));
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
}

void HttpServer::upgrade(HttpServerRequest *, QAbstractSocket *socket,
                         const QByteArray &)
{
    socket->close();
}

void HttpServer::onNewConnection(int socketDescriptor)
{
    incomingConnection(socketDescriptor);
}

void HttpServer::onRequestReady(Tufao::HttpServerResponse::Options options)
{
    HttpServerRequest *request = qobject_cast<HttpServerRequest *>(sender());
    // This shouldn't happen
    if (!request)
        return;

    QAbstractSocket *socket = request->connection();
    HttpServerResponse *response = new HttpServerResponse(socket,
                                                          options,
                                                          this);

    connect(socket, SIGNAL(disconnected()), response, SLOT(deleteLater()));
    connect(response, SIGNAL(finished()), response, SLOT(deleteLater()));

    emit requestReady(request, response);
}

} // namespace Tufao