#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) 
{
    QApplication app(argc, argv);

    const QIcon appIcon(QCoreApplication::applicationDirPath() + "/icon.ico");
    QFile styleFile(":/style.qss");

    styleFile.open(QIODevice::ReadOnly);
    app.setStyleSheet(styleFile.readAll());
    app.setWindowIcon(appIcon);

    MainWindow w;
    w.show();

    return app.exec();
}

//#include "core/math/CMatrix.h"
//#include "biblio/CArxivSource.h"
//#include "biblio/CBibAnalyzer.h"
//#include <iostream>
//
//int main()
//{
//	CArxiveSource arxive;
//    CBibAnalyzer analyzer(&arxive, 64, 32, 128);
//
//    std::vector<std::string> topics = {
//     "proton", "neutron", "electron"};
//
//    analyzer.Build(topics);
//    analyzer.Train(topics, 10);
//    
//    analyzer.SaveModel("model.bin");
//
//    std::cout << "\n=== TF-IDF ===\n";
//    auto resultsTFIDF = analyzer.Search("proton", 10);
//    for (const auto& article : resultsTFIDF)
//        std::cout << article.title << "\n";
//
//    std::cout << "\n=== Embedding ===\n";
//    auto resultsEmb = analyzer.SearchByEmbedding("proton", 10);
//    for (const auto& article : resultsEmb)
//        std::cout << article.title << "\n";
//
//	return 0;
//}