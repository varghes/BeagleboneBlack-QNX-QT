#include <qt4/QtGui/QApplication>
#include <qt4/QtGui/QPushButton>
#include <stdio.h>
#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[]){
	printf("Application Running... ");
	QApplication app(argc,argv);
	QPushButton Button1("Hello World");
	Button1.resize(180,90);
	Button1.show();
	return app.exec();

}
