#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCharts/QChartView>
#include <QtCore/QDateTime>
#include <QtCharts/QDateTimeAxis>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>
#include <QtCharts/QValueAxis>
#include <QtCore/qmath.h>
#include <QMap>
#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QTabWidget>

#include <samara/model/kernel/Model.hpp>

#include <QDebug>

QT_CHARTS_USE_NAMESPACE

//using namespace ogdf;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
//    showFullScreen();
//    auto const desktop(QApplication::desktop());
//    setGeometry(desktop->screenGeometry(2).adjusted(0,30,0,-50));
}

MainWindow::~MainWindow()
{
    delete ui;
}

static void format_dates(const model::models::ModelParameters& parameters,
                         std::string& begin, std::string& end)
{
    utils::DateTime::format_date(parameters.get < std::string >("BeginDate"),
                                 begin);
    utils::DateTime::format_date(parameters.get < std::string >("EndDate"),
                                 end);
}


void MainWindow::runSamara(const std::string& simulation, int /* verbose */){
//    samara::GlobalParameters globalParameters;
//    model::models::ModelParameters parameters;
//    utils::ParametersReader reader;
//    std::string begin;
//    std::string end;

//    reader.loadFromDatabase("06SB15-fev13-D1_SV21", parameters);
//    format_dates(parameters, begin, end);

//    globalParameters.modelVersion = parameters.get < std::string >("IdModele");
//    model::kernel::KernelModel* model = new model::kernel::KernelModel;
//    kernel::Simulator simulator(model, globalParameters);

//    simulator.attachView("global", new model::observer::GlobalView);
//    simulator.init(utils::DateTime::toJulianDayNumber(begin), parameters);
//    simulator.run(utils::DateTime::toJulianDayNumber(begin),
//                  utils::DateTime::toJulianDayNumber(end));
}

void MainWindow::run(const std::string& simulation, const model::models::ModelParameters &parameters, int /* verbose */)
{
    ecomeristem::GlobalParameters globalParameters;
    kernel::KernelModel* model = new kernel::KernelModel;
    kernel::Simulator simulator(model, globalParameters);
    std::string begin;
    std::string end;


    format_dates(parameters, begin, end);
    observer::PlantView * view = new observer::PlantView();
    simulator.attachView("plant", view);
    simulator.init(utils::DateTime::toJulianDayNumber(begin), parameters);
    simulator.run(utils::DateTime::toJulianDayNumber(begin),
                  utils::DateTime::toJulianDayNumber(end));

    qDebug() << "Simulation done.";
    displayData(view, QString::fromStdString(begin), QString::fromStdString(end));



}

void MainWindow::addChart(int row, int col, QLineSeries *series, QGridLayout * lay, QString name){
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->legend()->hide();
    chart->setTitle(name);

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(10);
    axisX->setFormat("dd-MM");
    //    axisX->setTitleText("Date");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%i");
    //    axisY->setTitleText("Value");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    QChartView *chartView = new QChartView(chart);
    chartView->setMaximumHeight(150);
    chartView->setRubberBand(QChartView::HorizontalRubberBand);
    chartView->setRenderHint(QPainter::Antialiasing);

    lay->addWidget(chartView, row, col);


}


QColor getColor(int i){
    double PHI = (1 + qSqrt(5))/2;
    double n = i * PHI - floor(i * PHI);
    int h = qFloor(n * 256);
    return QColor::fromHsv(h, 245, 245, 255);
}

void MainWindow::displayData(observer::PlantView * view, QString begin, QString end){

    QScrollArea *scrollArea = new QScrollArea;
    QWidget *client = new QWidget();
    scrollArea->setWidget(client);
    scrollArea->setWidgetResizable(true);

    QGridLayout * lay = new QGridLayout();
    client->setLayout(lay);
    lay->setSpacing(0);
    ui->tabWidget->addTab(scrollArea, "Variables");


    startDate = QDate::fromString(begin, Qt::ISODate);
    QDate endDate = QDate::fromString(end, Qt::ISODate);
    currentDate = startDate;
    qDebug() << startDate << endDate;
    const int numCol = 3;


    std::map<std::string, std::vector < std::pair < double, std::string > > > m = view->values();
    int j = 0;
    for(auto it = m.begin(); it != m.end(); ++it) {
//    for (int j = 0; j < params.length(); ++j) {
        QString param = QString::fromStdString(it->first);
        QLineSeries *series = new QLineSeries();
        series->setColor(getColor(j));
        for (int i = 0; i < startDate.daysTo(endDate); ++i) {
            double value = view->get(startDate.addDays(i).toJulianDay(),
                                     param.toLocal8Bit().constData());
            QDateTime momentInTime;
            momentInTime.setDate(startDate.addDays(i));
            series->append(momentInTime.toMSecsSinceEpoch(), value);
        }
        addChart(j/numCol,j%numCol,series,lay, param.mid(6));
        j++;
    }

    graph = new GraphView();
    ui->tabWidget->addTab(graph, "Execution");
    //    std::ofstream out("trace.txt");
    //    out << utils::Trace::trace().elements()
    //           .filter_time(startDate.addDays(20).toJulianDay())
    //           .filter_type(artis::utils::KERNEL)
    //           .to_string();
    //    out.close();
    displayGraph();
}

void MainWindow::mousePressEvent(QMouseEvent * event)
    {
/*      if(event->button() == Qt::LeftButton)
      {
          currentDate = currentDate.addDays(-1);
//        msgBox = new QMessageBox();
//        msgBox->setWindowTitle("Hello");
//        msgBox->setText("You Clicked Left Mouse Button");
//        msgBox->show();
      } else*/ if(event->button() == Qt::RightButton){
          currentDate = currentDate.addDays(1);
          displayGraph();
      }
    }


void MainWindow::displayGraph(){
    graph->clear();
    auto elements = utils::Trace::trace().elements()
            .filter_type(artis::utils::COMPUTE);
//            .filter_time(currentDate.toJulianDay())

    foreach (auto elt, elements) {
        qDebug() << elt.get_time() << QString::fromStdString(elt.get_model_name());
        qDebug() << QString::fromStdString(elt.get_comment());
    }


/************************************************************************/
    elements = utils::Trace::trace().elements()
            .filter_type(artis::utils::KERNEL);

//    qDebug() << currentDate.toString() << ": day" << startDate.daysTo(currentDate) << " - " << elements.size()/2 << "steps";
    QString previousNodeName = "";
    bool lastIsBefore = false;

//    qDebug() << "Starting parsing" << elements.size();
    foreach (auto elt, elements) {
        QString name = QString::fromStdString(elt.get_comment());
        name = name.replace("class model::kernel::Model/", "");
        name = name.replace("class ecomeristem::", "");
        name = name.replace("class ", "");
        name = name.replace("model::kernel", "");
        name = name.replace("::Model", "");
        name = name.replace("Model", "");

        if(name.startsWith(" - BEFORE - ")){
            name = name.replace(" - BEFORE - ", "");
            if(lastIsBefore && previousNodeName != name && !previousNodeName.isEmpty()){
//                qDebug() << "BeforeCluster" << previousNodeName;
            }

            lastIsBefore = true;
        } else {
            name = name.replace(" - AFTER - ", "");
            if(previousNodeName == name) {
                QStringList clusterNames = name.split("/");
                QString nodeName = clusterNames.last();
                clusterNames.pop_back();
                QString clusterName = clusterNames.join("/");
//                qDebug() << "AddNode" << nodeName << "of cluster" << clusterName;
                graph->addComputeNode(nodeName, clusterName);

            } else {
//                qDebug() << "CloseCluster" << name;
            }
            lastIsBefore = false;
        }
        previousNodeName = name;

        //        name = name.replace("ecomeristem::", "");
        //        name = name.replace("ecomeristem", "");
        //        qDebug() << name;
    }
}


#include <QFileDialog>
#include "parametersdatamodel.h"
#include "meteodatamodel.h"

void MainWindow::on_loadSimulationFileBtn_clicked()
{

//    QString filename = QFileDialog::getOpenFileName();
    QString dirName = QFileDialog::getExistingDirectory();
    reader.loadParametersFromFiles(dirName.toStdString(), parameters);

//    simulation = "sim_rice";
//    reader.loadParametersFromProstgresql(simulation, parameters);

    ParametersDataModel * paramModel = new ParametersDataModel(&parameters);
    ui->parametersTableView->setModel(paramModel);

    MeteoDataModel * meteoModel = new MeteoDataModel(&parameters);
    ui->meteoTableView->setModel(meteoModel);

    QList<int> list= ui->splitter->sizes();
    qDebug() << list;
    list.replace(0,this->width()*0.33);
    list.replace(1,this->width()*0.67);
    ui->splitter->setSizes(list);
}

void MainWindow::on_loadMeteoFileBtn_clicked()
{
}



void MainWindow::on_launchSimulationButton_clicked()
{


    simulation = "sim_rice";
//    reader.loadParametersFromProstgresql(simulation, parameters);
//    reader.loadParametersFromFiles("d:/Output", parameters);
    std::cout << "Simulation loaded. Starting." << std::endl;
    run(simulation, parameters, 0);
    std::cout << "Simulation done." << std::endl;
}

void MainWindow::on_testLoadButton_clicked()
{

}

void MainWindow::on_loadSimulationDBButton_clicked()
{
    QString dirName = "D/Samples/ecomeristem_og_testSample";
    reader.loadParametersFromFiles(dirName.toStdString(), parameters);
    ParametersDataModel * paramModel = new ParametersDataModel(&parameters);
    ui->parametersTableView->setModel(paramModel);

    MeteoDataModel * meteoModel = new MeteoDataModel(&parameters);
    ui->meteoTableView->setModel(meteoModel);

    QList<int> list= ui->splitter->sizes();
    qDebug() << list;
    list.replace(0,this->width()*0.33);
    list.replace(1,this->width()*0.67);
    ui->splitter->setSizes(list);
    on_launchSimulationButton_clicked();

}
