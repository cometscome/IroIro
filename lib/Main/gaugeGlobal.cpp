/*!
 * @file gaugeGlobal.cpp
 * @brief Declaration of the GaugeGlobal class
 *
 * Time-stamp: <2013-06-05 10:16:10 neo>
 */

#include "gaugeGlobal.hpp"
#include "gaugeConf.hpp" 
#include "include/errors.hpp"

void GaugeGlobal::initializeUnit(){
  GaugeConf_unit gconf(format);
  gconf.init_conf(data);
}

void GaugeGlobal::initializeRand(const RandNum& rand){
  GaugeConf_rand gconf(format,rand);
  gconf.init_conf(data);
}

void GaugeGlobal::initializeTxt(const std::string &Filename){
  GaugeConf_txt gconf(format,Filename);
  gconf.init_conf(data);
}

void GaugeGlobal::initializeBin(const std::string &Filename){
  GaugeConf_bin gconf(format,Filename);
  gconf.init_conf(data);
}

void GaugeGlobal::initializeCSDTbin(const std::string &Filename){
  GaugeConf_csdt gconf(format,Filename);
  gconf.init_conf(data);
}

void GaugeGlobal::initializeJLQCDlegacy(const std::string &Filename) {
  GaugeConf_JLQCDLegacy gconf(format,Filename);
  gconf.init_conf(data);
}

void GaugeGlobal::initializeNERSC(const std::string &Filename) {
  GaugeConf_NERSC gconf(format,Filename);
  gconf.init_conf(data);
}

void GaugeGlobal::initializeILDG(const std::string &Filename) {
  GaugeConf_ILDG gconf(format,Filename);
  gconf.init_conf(data);
}


int GaugeGlobal::initialize(XML::node node){
  try{
    XML::descend(node, "Configuration");
    if(!XML::attribute_compare(node,"Type","TextFile")){
      std::string filename(node.child_value());
      initializeTxt(filename);
      return 0;
    }
    if(!XML::attribute_compare(node,"Type","Unit")){
      initializeUnit();
      return 0;
    }
    if(!XML::attribute_compare(node,"Type","Binary")){
      std::string filename(node.child_value());
      initializeBin(filename);
      return 0;
    }
    if(!XML::attribute_compare(node,"Type","CSDTbinary")){
      std::string filename(node.child_value());
      initializeCSDTbin(filename);
      return 0;
    }
    if(!XML::attribute_compare(node,"Type","JLQCDlegacy")){
      std::string filename(node.child_value());
      initializeJLQCDlegacy(filename);
      return 0;
    }
    if(!XML::attribute_compare(node,"Type","NERSC")){
      std::string filename(node.child_value());
      initializeNERSC(filename);
      return 0;
    }
    if(!XML::attribute_compare(node,"Type","ILDG")){
      std::string filename(node.child_value());
      initializeILDG(filename);
      return 0;
    }

    Errors::XMLerr("Configuration type unknown");
  }catch(...) {
    Errors::XMLerr("Error in initialization of gauge field ");
  }
  return 0;
}

int GaugeGlobal::initialize(XML::node node,std::string filename){
  CCIO::cout<<"filename\n";
  try{
    XML::descend(node,"Configuration");
    if(!XML::attribute_compare(node,"Type","TextFile")){
      initializeTxt(filename);
      return 0;
    }
    if(!XML::attribute_compare(node,"Type","Binary")){
      initializeBin(filename);
      return 0;
    }
    if(!XML::attribute_compare(node,"Type","CSDTbinary")){
      initializeCSDTbin(filename);
      return 0;
    }
    if(!XML::attribute_compare(node,"Type","JLQCDlegacy")){
      initializeJLQCDlegacy(filename);
      return 0;
    }
    if(!XML::attribute_compare(node,"Type","NERSC")){
      initializeNERSC(filename);
      return 0;
    }
    if(!XML::attribute_compare(node,"Type","ILDG")){
      initializeILDG(filename);
      return 0;
    }

    std::cout << "Configuration type unknown\n";
    exit(1);
  }catch(...) {
    std::cout << "Error in initialization of gauge field "<< std::endl;
  }
  return 0;
}

