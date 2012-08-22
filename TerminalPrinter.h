#ifndef TERMINALPRINTER_H
#define TERMINALPRINTER_H

#include <QObject>
#include "JarvisClient.h"
#include "ModulePackage.h"
#include <QTextStream>

class TerminalPrinter : public QObject
{
    Q_OBJECT

private:
    JarvisClient &client;
    QTextStream qtout;
    QString currentScope;
    QMap<QString, Scope>  scopeByName;
    QStringList serverScopes;
    QList<ModulePackage> pkgs;
    void printPackage(const ModulePackage &pkg);

    void doPrintVars(const Scope &scope);
    void doPrintFuncs(const Scope &scope);
    void setCurrentScope(const QString &name) { currentScope = name; emit currentScopeChanged(name); }

public:
    explicit TerminalPrinter(JarvisClient &client);

signals:
    void output(const QString &);
    void currentScopeChanged(const QString &);
public slots:
    void newScope(const QString &name);
    void newFunction(const QString &scope, const QString &identifier, const QStringList &arguments, const QString &def);
    void newVariable(const QString &scope, const QString &identifier, const QString &definition);
    void newClient(const QString &scope, const QString &name);
    void clientLeft(const QString &scope, const QString &name);
    void msgInScope(const QString &scope, const QString &sender, const QString &msg);
    void error(JarvisClient::ClientError error);
    void pkgLoaded(const ModulePackage &pkg);
    void pkgUnloaded(const QString &name);
    void enteredScope(const QString &name, const Scope &info);
    void receivedInitInfo(const QStringList &scopes, const QList<ModulePackage> &pkgs);
    void openScope(const QString &name);
    void printClients();
    void printModules();
    void printVariables() { if (! currentScope.isEmpty()) doPrintVars(scopeByName[currentScope]); }
    void printFunctions() { if (! currentScope.isEmpty()) doPrintFuncs(scopeByName[currentScope]); }
    void leaveScope(const QString &name);
    void printScopes();
    void msgToScope(const QString &msg) { if (! currentScope.isEmpty()) QMetaObject::invokeMethod(&client, "msgToScope", Q_ARG(QString, currentScope), Q_ARG(QString, msg)); }
    void deletedScope(const QString &name);
    void disconnected() { emit output("Server died :/ u happy now? Send Arlecks to revive me... mb try \"jarc reconnect\""); }

};

#endif // TERMINALPRINTER_H
