#include <QApplication>
#include "DemoWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DualRecordDemo");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("BankSystem");

    DemoWindow w;
    w.show();
    return app.exec();
}
