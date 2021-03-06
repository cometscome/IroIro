/*! @file autoMap_improved.cpp
    @brief Definisions of member functions of AutoMap & AutoMap_EvenOdd which 
           assume internal indexing for the Fields.
 */

#include "autoMap.hpp"
#ifdef IBM_BGQ_WILSON
#include <omp.h>
#include "bgqthread.h"

#include <hwi/include/bqc/A2_inlines.h>
#endif

namespace Mapping{
  /////////////// AutoMap ///////////////
  /// for experimental use
  void AutoMap::operator()(GaugeField1D& Fout,const GaugeField1D& Fin,
			   Forward)const{
    std::valarray<double> recv_bdry(bdry_t_.size()*Fin.Nin());
    Communicator::instance()->transfer_fw(recv_bdry,
					  Fin.data[Fin.get_sub(bdry_b_)],dir_);
    Fout.data.set(Fin.get_sub(bdry_t_),recv_bdry);
    Fout.data.set(Fin.get_sub(bulk_t_),Fin.data[Fin.get_sub(bulk_b_)]);
  }

  void AutoMap::operator()(GaugeField1D& Fout,const GaugeField1D& Fin,
			   Backward)const{
    std::valarray<double> recv_bdry(bdry_b_.size()*Fin.Nin());
    Communicator::instance()->transfer_bk(recv_bdry,
					  Fin.data[Fin.get_sub(bdry_t_)],dir_);
    Fout.data.set(Fin.get_sub(bdry_b_),recv_bdry);
    Fout.data.set(Fin.get_sub(bulk_b_),Fin.data[Fin.get_sub(bulk_t_)]);
  }

  void AutoMap::operator()(GaugeField1D& Fout,const GaugeField& Fin,
			   int mu,Forward)const{
    std::valarray<double> recv_bdry(bdry_t_.size()*Fin.Nin());
    Communicator::instance()->transfer_fw(recv_bdry,
					  Fin.data[Fin.get_sub(bdry_b_,mu)],dir_);
    Fout.data.set(Fin.get_sub(bdry_t_),recv_bdry);
    Fout.data.set(Fin.get_sub(bulk_t_),Fin.data[Fin.get_sub(bulk_b_,mu)]);
  }

  void AutoMap::operator()(GaugeField1D& Fout,const GaugeField& Fin,
			   int mu,Backward)const{
    std::valarray<double> recv_bdry(bdry_b_.size()*Fin.Nin());
    Communicator::instance()->transfer_bk(recv_bdry,
					  Fin.data[Fin.get_sub(bdry_t_,mu)],dir_);
    Fout.data.set(Fin.get_sub(bdry_b_),recv_bdry);
    Fout.data.set(Fin.get_sub(bulk_b_),Fin.data[Fin.get_sub(bulk_t_,mu)]);
  }
  

  /////////////// AutoMap_EvenOdd ///////////////
  void AutoMap_EvenOdd::operator()(GaugeField1D& Fout,const double* Fin,
				   Forward)const{
#ifdef IBM_BGQ_WILSON
    if(!omp_in_parallel()){
#endif
      int Nin = Fout.Nin();
      int bdsize = Nin*send_bdry_t_.size();
      double send_bdry[bdsize],recv_bdry[bdsize];
      
      for(int b=0; b<send_bdry_t_.size(); ++b)
	for(int i=0; i<Nin; ++i)
	  send_bdry[b*Nin+i] = Fin[send_bdry_b_[b]*Nin+i];
      
      Communicator::instance()->transfer_fw(recv_bdry,send_bdry,bdsize,dir_);
      Communicator::instance()->sync();
      
      for(int b=0; b<recv_bdry_t_.size(); ++b)
	for(int i=0; i<Nin; ++i)
	  Fout.data.set(Fout.format.index(i,recv_bdry_t_[b]),
			recv_bdry[b*Nin+i]);
      
      for(int b=0; b<recv_bulk_b_.size(); ++b)
	for(int i=0; i<Nin; ++i)
	  Fout.data.set(Fout.format.index(i,recv_bulk_t_[b]),
			Fin[send_bulk_b_[b]*Nin+i]);
#ifdef IBM_BGQ_WILSON
    }else{
      //variables declared here are private by default                           
      int tID = omp_get_thread_num();
      int nID = omp_get_num_threads();
      int Nin = Fout.Nin();
      int block   = send_bdry_t_.size()/nID;
      int bulk_bl = send_bulk_b_.size()/nID;

      double* class_send = 
	(double*)BGQThread_Malloc(send_bdry_t_.size()*Nin*sizeof(double),nID);
      double* class_recv =
	(double*)BGQThread_Malloc(recv_bdry_t_.size()*Nin*sizeof(double),nID);
      
      for(int b=0; b<block; ++b)
	for(int i=0; i<Nin; ++i)
	  class_send[(b+tID*block)*Nin+i] = Fin[send_bdry_b_[b+tID*block]*Nin+i];
      
      BGQThread_Barrier(0, nID);
      
      if(tID == 0) 
	Communicator::instance()->transfer_fw(class_recv,class_send,
					      send_bdry_t_.size()*Nin,dir_);
      BGQThread_Barrier(0, nID);
      
      for(int b=0; b<block; ++b)
	for(int i=0; i<Nin; ++i)
	  Fout.data.set(Fout.format.index(i,recv_bdry_t_[b+tID*block]),
			class_recv[(b+tID*block)*Nin+i]);
	
      for(int b=0; b<bulk_bl; ++b)
	for(int i=0; i<Nin; ++i)
	  Fout.data.set(Fout.format.index(i,recv_bulk_t_[b+tID*bulk_bl]),
			Fin[send_bulk_b_[b+tID*bulk_bl]*Nin+i]);
	
      BGQThread_Barrier(0,nID);
      BGQThread_Free(class_send,tID);
      BGQThread_Free(class_recv,tID);
    }
#endif
  }
  
  void AutoMap_EvenOdd::operator()(GaugeField1D& Fout,const double* Fin,
				   Backward)const{
#ifdef IBM_BGQ_WILSON
    if(!omp_in_parallel()){
#endif
      int Nin = Fout.Nin();
      int bdsize = Nin*send_bdry_b_.size();
      double send_bdry[bdsize],recv_bdry[bdsize];
      
      for(int b=0; b<send_bdry_b_.size(); ++b)
	for(int i=0; i<Nin; ++i)
	  send_bdry[b*Nin+i] = Fin[send_bdry_t_[b]*Nin+i];
	
      Communicator::instance()->transfer_bk(recv_bdry,send_bdry,bdsize,dir_);
      Communicator::instance()->sync();
      
      for(int b=0; b<recv_bdry_b_.size(); ++b)
	for(int i=0; i<Nin; ++i)
	  Fout.data.set(Fout.format.index(i,recv_bdry_b_[b]),
			recv_bdry[b*Nin+i]);
      
      for(int b=0; b<recv_bulk_t_.size(); ++b)
	for(int i=0; i<Nin; ++i)
	  Fout.data.set(Fout.format.index(i,recv_bulk_b_[b]),
			Fin[send_bulk_t_[b]*Nin+i]);
#ifdef IBM_BGQ_WILSON      
    }else{
      //variables declared here are private by default                         
      int tID = omp_get_thread_num();
      int nID = omp_get_num_threads();
      int Nin = Fout.Nin();
      int block   = send_bdry_b_.size()/nID;
      int bulk_bl = send_bulk_t_.size()/nID;

      double* class_send =
	(double*)BGQThread_Malloc(send_bdry_b_.size()*Nin*sizeof(double),nID);
      double* class_recv =
	(double*)BGQThread_Malloc(recv_bdry_b_.size()*Nin*sizeof(double),nID);
      
      for(int b=0; b<block; ++b)
	for(int i=0; i<Nin; ++i)
	  class_send[(b+tID*block)*Nin+i] = Fin[send_bdry_t_[b+tID*block]*Nin+i];
      
      BGQThread_Barrier(0, nID);

      if(tID == 0) 
	Communicator::instance()->transfer_bk(class_recv,class_send,
					      send_bdry_b_.size()*Nin,dir_);
      BGQThread_Barrier(0, nID);
      
      for(int b=0; b<block; ++b)
	for(int i=0; i<Nin; ++i)
	  Fout.data.set(Fout.format.index(i,recv_bdry_b_[b+tID*block]),
			class_recv[(b+tID*block)*Nin+i]);
	
      for(int b=0; b<bulk_bl; ++b)
	for(int i=0; i<Nin; ++i)
	  Fout.data.set(Fout.format.index(i,recv_bulk_b_[b+tID*bulk_bl]),
			Fin[send_bulk_t_[b+tID*bulk_bl]*Nin+i]);

      BGQThread_Barrier(0,nID);
      BGQThread_Free(class_send,tID);
      BGQThread_Free(class_recv,tID);
    }
#endif
  }




}
