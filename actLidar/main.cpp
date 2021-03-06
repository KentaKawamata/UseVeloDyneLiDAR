#include <iostream>
#include <string>
#include <vector>
#include <boost/timer.hpp>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/io/vlp_grabber.h>
#include <pcl/console/parse.h>
#include <pcl/common/common_headers.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/io/ply_io.h>

// Point Type
// pcl::PointXYZ, pcl::PointXYZI, pcl::PointXYZRGBA
typedef pcl::PointXYZI PointType;


/*
void keyboardEvent(const pcl::visualization::KeyboardEvent &event,
                            pcl::PointCloud<PointType>::ConstPtr cloud) {

    if(event.getKeySym() == "space" && event.keyDown()){

        pcl::io::savePLYFileASCII("test.ply", *cloud);
        std::cout << "---------- SAVE DATA !!!!! ----------" << std::endl;
    }
        //next_iteration = true;
}
*/

int main( int argc, char *argv[] ) {

    // Command-Line Argument Parsing
    if( pcl::console::find_switch( argc, argv, "-help" ) ){
        std::cout << "usage: " << argv[0]
            << " [-ipaddress <192.168.1.70>]"
            << " [-port <2368>]"
            << " [-pcap <*.pcap>]"
            << " [-help]"
            << std::endl;
        return 0;
    }

    std::string ipaddress( "192.168.1.70" );
    std::string port( "2368" );
    std::string pcap;

    pcl::console::parse_argument( argc, argv, "-ipaddress", ipaddress );
    pcl::console::parse_argument( argc, argv, "-port", port );
    pcl::console::parse_argument( argc, argv, "-pcap", pcap );

    std::cout << "-ipadress : " << ipaddress << std::endl;
    std::cout << "-port : " << port << std::endl;
    std::cout << "-pcap : " << pcap << std::endl;

    // Point Cloud
    pcl::PointCloud<PointType>::ConstPtr cloud;

    // PCL Visualizer
    boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer( new pcl::visualization::PCLVisualizer( "Velodyne Viewer" ) );
    viewer->addCoordinateSystem( 3.0, "coordinate" );
    viewer->setBackgroundColor( 0.0, 0.0, 0.0, 0 );
    viewer->initCameraParameters();
    viewer->setCameraPosition( 0.0, 0.0, 30.0, 0.0, 1.0, 0.0, 0 );
    //viewer->registerKeyboardCallback(keyboardEvent, *cloud);

    // Point Cloud Color Hndler
    pcl::visualization::PointCloudColorHandler<PointType>::Ptr handler;
    const std::type_info& type = typeid( PointType );
    if( type == typeid( pcl::PointXYZ ) ){
        std::vector<double> color = { 255.0, 255.0, 255.0 };
        boost::shared_ptr<pcl::visualization::PointCloudColorHandlerCustom<PointType>> color_handler( new pcl::visualization::PointCloudColorHandlerCustom<PointType>( color[0], color[1], color[2] ) );
        handler = color_handler;
    }
    else if( type == typeid( pcl::PointXYZI ) ){
        boost::shared_ptr<pcl::visualization::PointCloudColorHandlerGenericField<PointType>> color_handler( new pcl::visualization::PointCloudColorHandlerGenericField<PointType>( "intensity" ) );
        handler = color_handler;
    }
    else if( type == typeid( pcl::PointXYZRGBA ) ){
        boost::shared_ptr<pcl::visualization::PointCloudColorHandlerRGBField<PointType>> color_handler( new pcl::visualization::PointCloudColorHandlerRGBField<PointType>() );
        handler = color_handler;
    }
    else{
        throw std::runtime_error( "This PointType is unsupported." );
    }

    // Retrieved Point Cloud Callback Function
    boost::mutex mutex;
    boost::function<void( const pcl::PointCloud<PointType>::ConstPtr& )> function =
        [ &cloud, &mutex ]( const pcl::PointCloud<PointType>::ConstPtr& ptr ){
            boost::mutex::scoped_lock lock( mutex );

            /* Point Cloud Processing */

            cloud = ptr;
        };

    // VLP Grabber
    boost::shared_ptr<pcl::VLPGrabber> grabber;
    if( !pcap.empty() ){
        std::cout << "Capture from PCAP..." << std::endl;
        grabber = boost::shared_ptr<pcl::VLPGrabber>( new pcl::VLPGrabber( pcap ) );
    }
    else if( !ipaddress.empty() && !port.empty() ){
        std::cout << "Capture from Sensor..." << std::endl;
        grabber = boost::shared_ptr<pcl::VLPGrabber>( new pcl::VLPGrabber( boost::asio::ip::address::from_string( ipaddress ), boost::lexical_cast<unsigned short>( port ) ) );
    }

    // Register Callback Function
    boost::signals2::connection connection = grabber->registerCallback( function );

    // Start Grabber
    grabber->start();
    boost::timer t;
    int num=0;

    while( !viewer->wasStopped() ){
        // Update Viewer
        viewer->spinOnce();

        if (t.elapsed() >= 5.0) { // 秒
            t.restart();
            std::string filename = "./data/Velodyne_" + std::to_string(num) + ".ply";
            pcl::io::savePLYFileASCII(filename, *cloud);
            num++;
        }

        boost::mutex::scoped_try_lock lock( mutex );
        if( lock.owns_lock() && cloud ){
            // Update Point Cloud
            handler->setInputCloud(cloud);
            if( !viewer->updatePointCloud( cloud, *handler, "cloud" ) ){
                viewer->addPointCloud( cloud, *handler, "cloud" );
            }
        }
    }
    //pcl::io::savePLYFileASCII("test.ply", *cloud);

    // Stop Grabber
    grabber->stop();

    // Disconnect Callback Function
    if( connection.connected() ){
        connection.disconnect();
    }

    return 0;
}
