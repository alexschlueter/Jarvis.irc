#include "TerminalPrinter.h"

TerminalPrinter::TerminalPrinter(JarvisClient &client) : client(client), qtout(stdout)
{
   connect(&client, SIGNAL(msgInScope(const QString &, const QString &, const QString &)), SLOT(msgInScope(const QString &, const QString &, const QString &)));
   connect(&client, SIGNAL(newFunction(const QString &, const QString &, const QStringList &, const QString &)), SLOT(newFunction(const QString &, const QString &, const QStringList &, const QString &)));
   connect(&client, SIGNAL(newScope(const QString &)), SLOT(newScope(const QString &)));
   connect(&client, SIGNAL(deletedScope(const QString &)), SLOT(deletedScope(const QString &)));
   connect(&client, SIGNAL(newVariable(const QString &, const QString &, const QString &)), SLOT(newVariable(const QString &, const QString &, const QString &)));
   connect(&client, SIGNAL(newClient(const QString &, const QString &)), SLOT(newClient(const QString &, const QString &)));
   connect(&client, SIGNAL(clientLeft(const QString &, const QString &)), SLOT(clientLeft(const QString &, const QString &)));
   connect(&client, SIGNAL(error(JarvisClient::ClientError)), SLOT(error(JarvisClient::ClientError)));
   connect(&client, SIGNAL(pkgLoaded(const QVariant &)), SLOT(pkgLoaded(const QVariant &)));
   connect(&client, SIGNAL(pkgUnloaded(const QString &)), SLOT(pkgUnloaded(const QString &)));
   connect(&client, SIGNAL(enteredScope(const QString &, const QVariant &)), SLOT(enteredScope(const QString &, const QVariant &)));
   connect(&client, SIGNAL(receivedInitInfo(const QVariant &, const QVariant &)), SLOT(receivedInitInfo(const QVariant &, const QVariant &)));
   connect(&client, SIGNAL(disconnected()), SLOT(disconnected()));
}

void TerminalPrinter::newScope(const QString &name)
{
    emit output("New Scope: " + name);
    serverScopes.append(name);
}

void TerminalPrinter::newFunction(const QString &scope, const QString &identifier, const QStringList &arguments, const QString &def)
{
    QString result = "New function definition (scope " + scope + "): " + identifier + "(" + arguments.front();
    for (QStringList::const_iterator it = arguments.begin() + 1; it != arguments.end(); ++it) result += "," + *it;
    emit output(result + ")=" + def);
    scopeByName[scope].functions.insert(identifier, qMakePair(arguments, def));
}

void TerminalPrinter::newVariable(const QString &scope, const QString &identifier, const QString &definition)
{
    emit output("New variable definition (scope " + scope + "): " + identifier + "=" + definition);
    scopeByName[scope].variables.insert(identifier, definition);
}

void TerminalPrinter::newClient(const QString &scope, const QString &name)
{
   emit output("New client (scope " + scope + "): " + name);
   scopeByName[scope].clients.append(name);
}

void TerminalPrinter::clientLeft(const QString &scope, const QString &name)
{
    emit output("Client left (scope " + scope + "): " + name);


   scopeByName[scope].clients.removeOne(name);
}

void TerminalPrinter::msgInScope(const QString &scope, const QString &sender, const QString &msg)
{
   emit output("[" + scope + "] " + sender + ": " + msg);


}

void TerminalPrinter::error(JarvisClient::ClientError error)
{
   emit output("Client Error " + QString::number(error));
}

void TerminalPrinter::pkgLoaded(const QVariant &pkg)
{
   emit output("Package loaded:");
   printPackage(pkg.value<ModulePackage>());


   pkgs.append(pkg.value<ModulePackage>());
}

void TerminalPrinter::pkgUnloaded(const QString &name)
{
   emit output("Package unloaded: " + name);


   pkgs.erase(std::remove_if(pkgs.begin(), pkgs.end(), [&](const ModulePackage &pkg) { return pkg.name == name; }));
}

void TerminalPrinter::enteredScope(const QString &name, const QVariant &info)
{
   Scope infoScope = info.value<Scope>();
   emit output("Entered scope " + name + "; Clients:");
   for (const auto &client : infoScope.clients) emit output(client + " ");
   emit output("Variables:");
   doPrintVars(infoScope);
   emit output("Functions:");
   doPrintFuncs(infoScope);

   scopeByName.insert(name, infoScope);
}

void TerminalPrinter::receivedInitInfo(const QVariant &scopes, const QVariant &pkgs)
{
   emit output("InitInfo:");
   emit output("Scopes:");
   for (const auto &scope : scopes.value<QStringList>()) {
       emit output(scope + " ");
       serverScopes.append(scope);
   }
   emit output("Packages:");
   for (const auto &pkg : pkgs.value<QList<ModulePackage> >()) {
       printPackage(pkg);
   }
   this->pkgs = pkgs.value<QList<ModulePackage> >();


}

void TerminalPrinter::printClients()
{
    if (! currentScope.isEmpty())
        for (const auto &client : scopeByName[currentScope].clients) emit output(client + " ");
}

void TerminalPrinter::printScopes()
{
    for (const auto &scope : serverScopes) emit output(scope + " ");


}

void TerminalPrinter::deletedScope(const QString &name)
{
    emit output("Deleted scope " + name);
    if (currentScope == name) setCurrentScope(QString());
    scopeByName.remove(name);
    serverScopes.removeOne(name);
}

void TerminalPrinter::printModules()
{
    for (const auto &pkg : pkgs) printPackage(pkg);
}

void TerminalPrinter::leaveScope(const QString &name)
{
    if (scopeByName.contains(name)) {
        scopeByName.remove(name);
        if (currentScope == name) setCurrentScope(QString());
        client.leaveScope(name);
        emit output("Left scope " + name);
    } else emit output("I'm not in a scope called" + name);
}

void TerminalPrinter::printPackage(const ModulePackage &pkg)
{
    emit output("Package Name Module Name Module Description");
    emit output(pkg.name);
    emit output(" Terminals:");
    for (const auto &mod : pkg.terminals) {
        emit output("  " + mod.name + " " + mod.description);
    }
    emit output(" Operators:");
    for (const auto &mod : pkg.operators) {
        emit output("  " + mod.name + " " + mod.description);
    }
    emit output(" Functions:");
    for (const auto &mod : pkg.functions) {
        emit output("  " + mod.name + " " + mod.description);
    }

}

void TerminalPrinter::doPrintVars(const Scope &scope)
{
    for (auto it = scope.variables.begin(); it != scope.variables.end(); ++it) emit output(it.key() + "=" + it.value());
}

void TerminalPrinter::doPrintFuncs(const Scope &scope)
{
    for (auto it = scope.functions.begin(); it != scope.functions.end(); ++it) {
        QString result = it.key() + "(" + it.value().first.front();
        for (auto it_args = it.value().first.begin() + 1; it_args != it.value().first.end(); ++it_args) result += "," + *it_args;
        emit output(result + ")=" + it.value().second);
    }
}

void TerminalPrinter::openScope(const QString &name)
{
    if (scopeByName.contains(name)) setCurrentScope(name);
    else emit output("Enter the scope before opening it.");
}