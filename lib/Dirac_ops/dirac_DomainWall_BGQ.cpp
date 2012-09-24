/*!--------------------------------------------------------------------------
 * @file dirac_DomainWall_BGQ.cpp
 *
 * @brief Definition of some BGQ optimized methods for Dirac_optimalDomainWall (5d operator)
 *
 *-------------------------------------------------------------------------*/
#include "dirac_DomainWall.hpp"
#include "Solver/solver.hpp"

#ifdef IBM_BGQ_WILSON
#include "bgqwilson.h"
#include "bgqthread.h"
#include <omp.h>
#include <hwi/include/bqc/A2_inlines.h>
#endif

typedef struct FermionSpinor{
  double _Complex v[12];
}Spinor;
typedef struct GaugeConf{
  double _Complex v[9];
}GaugePtr;


#define ENABLE_THREADING

double omp_norm(Spinor* pointer, int is, int ns, int nid, int tid){
	double tSum;

	BGWilsonLA_Norm(&tSum,pointer+is,ns);

	tSum = BGQThread_GatherDouble(tSum,0,tid,nid);
	if(tid == 0){
		tSum = Communicator::instance()->reduce_sum(tSum);
	}
	tSum = BGQThread_ScatterDouble(tSum,0,tid,nid);

	return sqrt(tSum);
}


void Dirac_optimalDomainWall::mult_hop(Field& w5, const Field& f5) const{
  typedef struct FermionSpinor{
    double _Complex v[12];
  }Spinor;
  typedef struct GaugeConf{
    double _Complex v[9];
  }GaugePtr;

  Field lpf(f4size_);
  Field lmf(f4size_), ey(f4size_), fy(f4size_);
  Field temp(fsize_);

  Spinor* w_ptr    = (Spinor*)malloc(sizeof(double)*f4size_);
  Spinor* v_ptr    = (Spinor*)malloc(sizeof(double)*f4size_);

  Spinor* lpf_ptr  = (Spinor*)lpf.getaddr(0);
  Spinor* lmf_ptr  = (Spinor*)lmf.getaddr(0);
  Spinor* ey_ptr   = (Spinor*)ey.getaddr(0);
  Spinor* fy_ptr   = (Spinor*)fy.getaddr(0);
  Spinor* temp_ptr = (Spinor*)temp.getaddr(0);
  Spinor* temp_ptr_bdry = (Spinor*)temp.getaddr((N5_-1)*f4size_);
  Spinor* w5_ptr_bdry   = (Spinor*)w5.getaddr((N5_-1)*f4size_);

  GaugePtr* pU       = (GaugePtr*)(const_cast<Field *>(u_)->getaddr(0));
  Spinor* f5_ptr     = (Spinor*)(const_cast<Field&>(f5).getaddr(0));

  double mass_fact   = 4.0+M0_;

  double minus_kappa = -Dw_.getKappa();
  double doe_factor = minus_kappa*mass_fact;
  double fact= 1.0/(Params.dp_[N5_-1] +mq_*Params.dm_[N5_-2]*Params.es_[N5_-2]);

  register int Nvol = CommonPrms::instance()->Nvol()/2;

  Format::Format_F Fformat = Dw_.get_fermionFormat();

  int tid, nid;
  int is, ns;


  ///////////////////////////////  doe
#pragma omp parallel private(tid,nid,is,ns) firstprivate(f5_ptr, temp_ptr)
  {
    nid = omp_get_num_threads();
    tid = omp_get_thread_num();
    is = (Nvol * tid / nid);
    ns = Nvol/nid;
    
    // s = 0
    BGWilsonLA_Proj_P(lpf_ptr + is,const_cast<Field&>(f5).getaddr(Fformat.index(0,0,N5_-1))+is,ns);
    BGWilsonLA_Proj_M(lmf_ptr + is,const_cast<Field&>(f5).getaddr(Fformat.index(0,0,1))+is,ns);
    BGWilsonLA_MultScalar_Add(lpf_ptr + is,lmf_ptr + is,-mq_,ns);
    BGWilsonLA_AXPBY(v_ptr + is, f5_ptr + is, lpf_ptr + is, Params.bs_[0], Params.cs_[0], ns);

    BGWilson_MultEO(temp_ptr, pU, v_ptr, doe_factor , 2, BGWILSON_DIRAC);

    for(int s=1; s<N5_-1; ++s) {
      f5_ptr   += Nvol;
      temp_ptr += Nvol;
      
      BGWilsonLA_Proj_P(lpf_ptr+is,const_cast<Field&>(f5).getaddr(Fformat.index(0,0,s-1))+is,ns);
      BGWilsonLA_Proj_M(lmf_ptr+is,const_cast<Field&>(f5).getaddr(Fformat.index(0,0,s+1))+is,ns);
      BGWilsonLA_Add(lpf_ptr + is,lmf_ptr + is ,ns);
      BGWilsonLA_AXPBY(v_ptr + is , f5_ptr + is, lpf_ptr + is , Params.bs_[s],Params.cs_[s],ns);
     
      BGWilson_MultEO(temp_ptr, pU, v_ptr, doe_factor , 2, BGWILSON_DIRAC);
    }
 
 
    // s = N5-1
    f5_ptr   += Nvol;
    temp_ptr += Nvol;
    
    BGWilsonLA_Proj_P(lpf_ptr+is,const_cast<Field&>(f5).getaddr(Fformat.index(0,0,N5_-2))+is,ns);
    BGWilsonLA_Proj_M(lmf_ptr+is,const_cast<Field&>(f5).getaddr(Fformat.index(0,0,0))+is,ns);
    BGWilsonLA_MultAddScalar(lpf_ptr+is,lmf_ptr+is,-mq_,ns);  
    BGWilsonLA_AXPBY(v_ptr+is, f5_ptr+is, lpf_ptr+is, Params.bs_[N5_-1],Params.cs_[N5_-1],ns);

    BGWilson_MultEO(temp_ptr, pU, v_ptr, doe_factor , 2, BGWILSON_DIRAC); 


    /// 5d hopping term 

    //time1 = GetTimeBase();

    BGWilsonLA_Proj_M(ey_ptr+is,(Spinor*)temp.getaddr(0)+is,ns);
    BGWilsonLA_MultAddScalar(temp_ptr_bdry+is,     ey_ptr+is,-mq_* Params.es_[0],ns);

    for (int s=1; s<N5_-1; ++s) {
      Spinor* temp_ptr   = (Spinor*)temp.getaddr(s*f4size_);
      double fact_lpf = (Params.dm_[s]/Params.dp_[s-1]);
      double fact_ey =  mq_*Params.es_[s];
      
      BGWilsonLA_Proj_P(lpf_ptr+is ,(Spinor*)temp.getaddr(Fformat.index(0,0,s-1))+is ,ns);
      BGWilsonLA_Proj_M(ey_ptr+is  ,(Spinor*)temp.getaddr(Fformat.index(0,0,s))+is   ,ns);
      
      BGWilsonLA_MultAddScalar(temp_ptr+is     ,lpf_ptr+is, fact_lpf,ns);
      BGWilsonLA_MultAddScalar(temp_ptr_bdry+is,ey_ptr+is, -fact_ey,ns);

    }
    BGWilsonLA_Proj_P(lpf_ptr+is,(Spinor*)temp.getaddr(Fformat.index(0,0,N5_-2))+is,ns);
    BGWilsonLA_MultAddScalar(temp_ptr_bdry+is,     lpf_ptr+is,(Params.dm_[N5_-1]/Params.dp_[N5_-2]),ns);
    
    BGWilsonLA_MultScalar(temp_ptr_bdry+is, temp_ptr_bdry+is, fact, ns);
    
    for(int s=N5_-2; s>=0; --s) {
      Spinor* temp_ptr   = (Spinor*)temp.getaddr(s*f4size_);
      BGWilsonLA_Proj_M(lmf_ptr+is,(Spinor*)temp.getaddr(Fformat.index(0,0,s+1))+is,ns);
      BGWilsonLA_Proj_P(fy_ptr+is,(Spinor*)temp.getaddr(Fformat.index(0,0,N5_-1))+is,ns);
      BGWilsonLA_MultAddScalar(temp_ptr+is,     lmf_ptr+is,Params.dm_[s],ns);
      BGWilsonLA_MultAddScalar(temp_ptr+is,     fy_ptr+is,-mq_*Params.fs_[s],ns);
      BGWilsonLA_MultScalar(temp_ptr+is, temp_ptr+is, 1.0/ Params.dp_[s], ns);
    }

    ////////////////deo
    for(int s=0; s<N5_; ++s) {
      Spinor* temp_ptr   = (Spinor*)temp.getaddr(s*f4size_);
      Spinor* w5_ptr   = (Spinor*)w5.getaddr(s*f4size_);
      BGWilsonLA_Proj_P(lpf_ptr+is,(Spinor*)temp.getaddr(Fformat.index(0,0,(s+N5_-1)%N5_))+is,ns);
      BGWilsonLA_Proj_M(lmf_ptr+is,(Spinor*)temp.getaddr(Fformat.index(0,0,(s+1)%N5_))+is,ns);
      
      if(s==0){
	BGWilsonLA_MultScalar_Add(lpf_ptr+is,lmf_ptr+is,-mq_,ns);
      }
      else if(s==N5_-1){
	BGWilsonLA_MultAddScalar(lpf_ptr+is,lmf_ptr+is,-mq_,ns);
      }
      else{
	BGWilsonLA_Add(lpf_ptr+is,lmf_ptr+is,ns);
      }
      BGWilsonLA_AXPBY(v_ptr+is, temp_ptr+is, lpf_ptr+is,
		       Params.bs_[s],Params.cs_[s],ns);
      
      BGWilson_MultEO(w_ptr, pU, v_ptr, minus_kappa , 1, BGWILSON_DIRAC);
      
      BGWilsonLA_MultScalar(w5_ptr+is, w_ptr+is, mass_fact, ns);
    }

    BGWilsonLA_Proj_M(ey_ptr+is,(Spinor*)w5.getaddr(Fformat.index(0,0,0))+is,ns);
    BGWilsonLA_MultAddScalar(w5_ptr_bdry+is,     ey_ptr+is,-mq_* Params.es_[0],ns);
    
    for (int s=1; s<N5_-1; ++s) {
      Spinor* w5_ptr   = (Spinor*)w5.getaddr(s*f4size_);
      double fact_lpf = (Params.dm_[s]/Params.dp_[s-1]);
      double fact_ey =  mq_*Params.es_[s];
      
      BGWilsonLA_Proj_P(lpf_ptr+is,(Spinor*)w5.getaddr(Fformat.index(0,0,s-1))+is,ns);
      BGWilsonLA_Proj_M(ey_ptr+is,(Spinor*)w5.getaddr(Fformat.index(0,0,s))+is,ns);
      
      BGWilsonLA_MultAddScalar(w5_ptr+is,     lpf_ptr+is,fact_lpf,ns);
      BGWilsonLA_MultAddScalar(w5_ptr_bdry+is,ey_ptr+is,-fact_ey,ns);
      
    }


      
    BGWilsonLA_Proj_P(lpf_ptr+is,(Spinor*)w5.getaddr(Fformat.index(0,0,N5_-2))+is,ns);
    BGWilsonLA_MultAddScalar(w5_ptr_bdry+is,     lpf_ptr+is,(Params.dm_[N5_-1]/Params.dp_[N5_-2]),ns);
    BGWilsonLA_MultScalar(w5_ptr_bdry+is, w5_ptr_bdry+is, fact, ns);
    
    for(int s=N5_-2; s>=0; --s) {
      Spinor* w5_ptr   = (Spinor*)w5.getaddr(s*f4size_);
      BGWilsonLA_Proj_M(lmf_ptr+is  ,(Spinor*)w5.getaddr(Fformat.index(0,0,s+1))+is,ns);
      BGWilsonLA_Proj_P(fy_ptr+is   ,(Spinor*)w5.getaddr(Fformat.index(0,0,N5_-1))+is,ns);
      BGWilsonLA_MultAddScalar(w5_ptr+is,     lmf_ptr+is,Params.dm_[s],ns);
      BGWilsonLA_MultAddScalar(w5_ptr+is,     fy_ptr+is,-mq_*Params.fs_[s],ns);
      BGWilsonLA_MultScalar(w5_ptr+is, w5_ptr+is, 1.0/ Params.dp_[s], ns);
    }
     
    //BGWilsonLA_MultScalar_Add((Spinor*)(w5.getaddr(0))+is*N5_,(Spinor*)(const_cast<Field&>(f5).getaddr(0))+is*N5_, -1.0, ns*N5_);
  
    Spinor* f5_ptr   = (Spinor*)(const_cast<Field&>(f5).getaddr(0));
    Spinor* w5_ptr   = (Spinor*)w5.getaddr(0);
    for(int s=0; s<N5_; ++s) {
      BGWilsonLA_MultScalar_Add(w5_ptr+is,f5_ptr+is, -1.0, ns);
      w5_ptr += Nvol;
      f5_ptr += Nvol;
    }    
  
  }  

  free(v_ptr);
  free(w_ptr);

}
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_THREADING
void Dirac_optimalDomainWall::mult_hop_omp(Field& w5, const void* f5) const
{
	int tid, nid;
	nid = omp_get_num_threads();
	tid = omp_get_thread_num();

	BGQThread_Barrier(0, nid);
	if(tid == 0){
		BGWilson_DW_Init(N5_,(double*)&Params.dp_[0],(double*)&Params.dm_[0],(double*)&Params.bs_[0],(double*)&Params.cs_[0],(double*)&Params.es_[0],(double*)&Params.fs_[0]);
	}
	BGQThread_Barrier(0, nid);

	BGWilson_DW_Mult_hop(w5.getaddr(0),(void*)(const_cast<Field *>(u_)->getaddr(0)),(void*)f5,
							Dw_.getKappa(),mq_,M0_,BGWILSON_DIRAC);

	/*
  //Allocate fields
  int tid, nid;
  nid = omp_get_num_threads();
  tid = omp_get_thread_num();

  BGQThread_Barrier(0, nid);
  void* lpf_ptr  = BGQThread_Malloc(sizeof(double)*f4size_, nid);
  void* lmf_ptr  = BGQThread_Malloc(sizeof(double)*f4size_, nid);
  BGQThread_Barrier(0, nid);
    
  mult_hop_omp_allocated(w5, f5, lpf_ptr, lmf_ptr,nid, tid);
    
  //Free fields
  BGQThread_Barrier(0, nid);
//  if (tid==0){
//    free(lpf_ptr);
//    free(lmf_ptr);
//  }
	BGQThread_Free(lpf_ptr,tid);
	BGQThread_Free(lmf_ptr,tid);
  BGQThread_Barrier(0, nid);
	*/
}


void Dirac_optimalDomainWall::mult_hop_omp_allocated(Field& w5, 
						     const void* f5,
						     void* lpf_ptr_void,
						     void* lmf_ptr_void,
						     int nid,
						     int tid) const
{

	/*
	BGWilson_DW_Mult_hop(w5.getaddr(0),(void*)(const_cast<Field *>(u_)->getaddr(0)),(void*)f5,
							Dw_.getKappa(),mq_,M0_,BGWILSON_DIRAC);
	*/


  register int Nvol = CommonPrms::instance()->Nvol()/2;
  //int tid, nid;
  int is, ie, ns;
  is = (Nvol * tid / nid);
  ie = (Nvol * (tid + 1) / nid);
  ns = ie - is;

  // 4d vectors
  Spinor* lpf_ptr = (Spinor*)lpf_ptr_void;
  Spinor* lmf_ptr = (Spinor*)lmf_ptr_void;

  // 5d vectors
  Spinor* temp_ptr_base = (Spinor*)BGQThread_Malloc(sizeof(double)*fsize_,nid);

  BGQThread_Barrier(0,nid);

  Spinor* temp_ptr = temp_ptr_base;
  Spinor* temp_ptr_bdry = temp_ptr_base+(N5_-1)*(Nvol);
  Spinor* w5_ptr_base   = (Spinor*)w5.getaddr(0);
  Spinor* w5_ptr_bdry   = (Spinor*)w5.getaddr((N5_-1)*f4size_);

  GaugePtr* pU       = (GaugePtr*)(const_cast<Field *>(u_)->getaddr(0));
  Spinor* f5_ptr        = (Spinor*)f5;
  Spinor* f5_ptr_base   = f5_ptr;

  double mass_fact   = 4.0+M0_;

  double minus_kappa = -Dw_.getKappa();
  double doe_factor = minus_kappa*mass_fact;
  double fact= 1.0/(Params.dp_[N5_-1] +mq_*Params.dm_[N5_-2]*Params.es_[N5_-2]);

  BGQThread_Barrier(0,nid);  

  ///////////////////////////////  doe
  // s = 0
  BGWilsonLA_Proj_P(lpf_ptr + is,(f5_ptr+Nvol*(N5_-1))+is, ns);
  BGWilsonLA_Proj_M(lmf_ptr + is,(f5_ptr+Nvol)        +is, ns);
  BGWilsonLA_MultScalar_Add(lpf_ptr + is,lmf_ptr + is,-mq_,ns);
  BGWilsonLA_AXPBY(lmf_ptr + is, f5_ptr + is, lpf_ptr + is, 
		   Params.bs_[0], Params.cs_[0], ns);
  
  BGWilson_MultEO(temp_ptr, pU, lmf_ptr, doe_factor , 2, BGWILSON_DIRAC);
 
 
  double norm = omp_norm(temp_ptr_base, is*N5_, ns*N5_, nid, tid);
  if (tid==0)
    CCIO::cout << "1- norm :" << norm << "\n";
   
  
  for(int s=1; s<N5_-1; ++s) {
    f5_ptr   += Nvol;
    temp_ptr += Nvol;
      
    BGWilsonLA_Proj_P(lpf_ptr+is,(f5_ptr_base + Nvol*(s-1)) + is, ns);
    BGWilsonLA_Proj_M(lmf_ptr+is,(f5_ptr_base + Nvol*(s+1)) + is, ns);
    BGWilsonLA_Add(lpf_ptr + is,lmf_ptr + is ,ns);
    BGWilsonLA_AXPBY(lmf_ptr + is , f5_ptr + is, lpf_ptr + is , 
		     Params.bs_[s],Params.cs_[s],ns);
     
    BGWilson_MultEO(temp_ptr, pU, lmf_ptr, doe_factor , 2, BGWILSON_DIRAC);
  }

  norm = omp_norm(temp_ptr_base, is*N5_, ns*N5_, nid, tid);
  if (tid==0)
    CCIO::cout << "2- norm :" << norm<< "\n";
 
  // s = N5-1
  f5_ptr   += Nvol;
  temp_ptr += Nvol;
    
  BGWilsonLA_Proj_P(lpf_ptr+is,(f5_ptr_base + Nvol*(N5_-2)) +is, ns);
  BGWilsonLA_Proj_M(lmf_ptr+is, f5_ptr_base                 +is, ns);
  BGWilsonLA_MultAddScalar(lpf_ptr+is,lmf_ptr+is,-mq_,ns);  
  BGWilsonLA_AXPBY(lmf_ptr+is, f5_ptr+is, lpf_ptr+is,
		   Params.bs_[N5_-1],Params.cs_[N5_-1],ns);

  BGWilson_MultEO(temp_ptr, pU, lmf_ptr, doe_factor , 2, BGWILSON_DIRAC); 

  norm = omp_norm(temp_ptr_base, is*N5_, ns*N5_, nid, tid);
  if (tid==0)
    CCIO::cout << "3- norm :" << norm<< "\n";

  //////////////////////////////////////////////////////////////////////////////////
  /// 5d hopping term 
  BGWilsonLA_Proj_M(lmf_ptr+is,temp_ptr_base+is,ns);
  BGWilsonLA_MultAddScalar(temp_ptr_bdry+is,     lmf_ptr+is,-mq_* Params.es_[0],ns);

  norm = omp_norm(temp_ptr_base, is*N5_, ns*N5_, nid, tid);
  if (tid==0)
    CCIO::cout << "4- norm :" << norm<< "\n";


  for (int s=1; s<N5_-1; ++s) {
    double fact_lpf = (Params.dm_[s]/Params.dp_[s-1]);
    double fact_ey =  mq_*Params.es_[s];
      
    BGWilsonLA_Proj_P(lpf_ptr+is  ,temp_ptr_base +(s-1)*Nvol +is ,ns);
    BGWilsonLA_Proj_M(lmf_ptr+is  ,temp_ptr_base +    s*Nvol +is ,ns);
      
    BGWilsonLA_MultAddScalar(temp_ptr_base + s*Nvol +is, lpf_ptr + is, fact_lpf, ns);
    BGWilsonLA_MultAddScalar(temp_ptr_bdry +         is, lmf_ptr + is, -fact_ey, ns);
  }

  BGWilsonLA_Proj_P(lpf_ptr+is,temp_ptr_base+(N5_-2)*Nvol+is,ns);
  BGWilsonLA_MultAddScalar(temp_ptr_bdry+is,     lpf_ptr+is,(Params.dm_[N5_-1]/Params.dp_[N5_-2]),ns);
    
  BGWilsonLA_MultScalar(temp_ptr_bdry+is, temp_ptr_bdry+is, fact, ns);

  norm = omp_norm(temp_ptr_base, is*N5_, ns*N5_, nid, tid);
  if (tid==0)
    CCIO::cout << "5- norm :" << norm<< "\n";

    
  for(int s=N5_-2; s>=0; --s) {
    temp_ptr   = temp_ptr_base+s*Nvol;
    BGWilsonLA_Proj_M(lmf_ptr+is,temp_ptr_base+(s+1)*Nvol+is,ns);
    BGWilsonLA_Proj_P(lpf_ptr+is,temp_ptr_base+(N5_-1)*Nvol+is,ns);
    BGWilsonLA_MultAddScalar(temp_ptr+is,     lmf_ptr+is,Params.dm_[s],ns);
    BGWilsonLA_MultAddScalar(temp_ptr+is,     lpf_ptr+is,-mq_*Params.fs_[s],ns);
    BGWilsonLA_MultScalar(temp_ptr+is, temp_ptr+is, 1.0/ Params.dp_[s], ns);
  }

  norm = omp_norm(temp_ptr_base, is*N5_, ns*N5_, nid, tid);
  if (tid==0)
    CCIO::cout << "6- norm :" << norm<< "\n";


  //////////////////////////////////////////////// deo
  for(int s=0; s<N5_; ++s) {
    Spinor* temp_ptr   = temp_ptr_base+s*Nvol;
    Spinor* w5_ptr   = w5_ptr_base + s*Nvol;
    BGWilsonLA_Proj_P(lpf_ptr+is,temp_ptr_base+((s+N5_-1)%N5_)*Nvol+is,ns);
    BGWilsonLA_Proj_M(lmf_ptr+is,temp_ptr_base+((s+1)%N5_)*Nvol+is,ns);
      
    if(s==0){
      BGWilsonLA_MultScalar_Add(lpf_ptr+is,lmf_ptr+is,-mq_,ns);
    }
    else if(s==N5_-1){
      BGWilsonLA_MultAddScalar(lpf_ptr+is,lmf_ptr+is,-mq_,ns);
    }
    else{
      BGWilsonLA_Add(lpf_ptr+is,lmf_ptr+is,ns);
    }
    BGWilsonLA_AXPBY(lmf_ptr+is, temp_ptr+is, lpf_ptr+is,
		     Params.bs_[s],Params.cs_[s],ns);
      
    BGWilson_MultEO(w5_ptr, pU, lmf_ptr, doe_factor , 1, BGWILSON_DIRAC);

  }

  norm = omp_norm(w5_ptr_base, is*N5_, ns*N5_, nid, tid);
  if (tid==0)
    CCIO::cout << "7- norm :" << norm<< "\n";


  BGWilsonLA_Proj_M(lmf_ptr+is,w5_ptr_base+is,ns);
  BGWilsonLA_MultAddScalar(w5_ptr_bdry+is,     lmf_ptr+is,-mq_* Params.es_[0],ns);
    
  for (int s=1; s<N5_-1; ++s) {
    Spinor* w5_ptr   = w5_ptr_base + s*Nvol;
    double fact_lpf = (Params.dm_[s]/Params.dp_[s-1]);
    double fact_ey =  mq_*Params.es_[s];
      
    BGWilsonLA_Proj_P(lpf_ptr+is, w5_ptr_base + Nvol*(s-1) +is, ns);
    BGWilsonLA_Proj_M(lmf_ptr+is , w5_ptr_base + Nvol*s     +is, ns);
 
    BGWilsonLA_MultAddScalar(w5_ptr+is      , lpf_ptr+is ,fact_lpf,ns);
    BGWilsonLA_MultAddScalar(w5_ptr_bdry+is , lmf_ptr+is  ,-fact_ey,ns);
  }
   
  norm = omp_norm(w5_ptr_base, is*N5_, ns*N5_, nid, tid);
  if (tid==0)
    CCIO::cout << "8- norm :" << norm<< "\n";

   
  BGWilsonLA_Proj_P(lpf_ptr+is,w5_ptr_base + Nvol*(N5_-2)+is,ns);
  BGWilsonLA_MultAddScalar(w5_ptr_bdry+is,     lpf_ptr+is,(Params.dm_[N5_-1]/Params.dp_[N5_-2]),ns);
  BGWilsonLA_MultScalar(w5_ptr_bdry+is, w5_ptr_bdry+is, fact, ns);
    
  for(int s=N5_-2; s>=0; --s) {
    Spinor* w5_ptr   = w5_ptr_base + s*Nvol;
    BGWilsonLA_Proj_M(lmf_ptr+is  ,w5_ptr_base + Nvol*(s+1)   + is, ns);
    BGWilsonLA_Proj_P(lpf_ptr+is   ,w5_ptr_base + Nvol*(N5_-1) + is, ns);
    BGWilsonLA_MultAddScalar(w5_ptr+is,     lmf_ptr+is,Params.dm_[s],ns);
    BGWilsonLA_MultAddScalar(w5_ptr+is,     lpf_ptr+is,-mq_*Params.fs_[s],ns);
    BGWilsonLA_MultScalar(w5_ptr+is, w5_ptr+is, 1.0/ Params.dp_[s], ns);
  }
 
  norm = omp_norm(w5_ptr_base, is*N5_, ns*N5_, nid, tid);
  if (tid==0)
    CCIO::cout << "9- norm :" << norm<< "\n";
    
  //BGWilsonLA_MultScalar_Add((Spinor*)(w5.getaddr(0))+is*N5_,(Spinor*)(const_cast<Field&>(f5).getaddr(0))+is*N5_, -1.0, ns*N5_);
  
  f5_ptr   = (Spinor*)f5;
  Spinor* w5_ptr   = w5_ptr_base;
  for(int s=0; s<N5_; ++s) {
    BGWilsonLA_MultScalar_Add(w5_ptr+is,f5_ptr+is, -1.0, ns);
    w5_ptr += Nvol;
    f5_ptr += Nvol;
  }    

  norm = omp_norm(w5_ptr_base, is*N5_, ns*N5_, nid, tid);
  if (tid==0)
    CCIO::cout << "10- norm :" << norm<< "\n";

  
  BGQThread_Barrier(0, nid);
  
//  if (tid==0)
//    free(temp_ptr_base);
	BGQThread_Free(temp_ptr_base,tid);
    
  
  BGQThread_Barrier(0, nid);
  

}


#endif


void Dirac_optimalDomainWall::mult_hop_dag(Field& w5, const Field& f5) const{
  typedef struct FermionSpinor{
    double _Complex v[12];
  }Spinor;
  typedef struct GaugeConf{
    double _Complex v[9];
  }GaugePtr;

  register int Nvol = CommonPrms::instance()->Nvol()/2;
  Field lpf(f4size_), ey(f4size_), lmf(f4size_), v(f4size_), w(f4size_);
  Field temp(f5),v5(fsize_);

  Spinor* w5_ptr  = (Spinor*)w5.getaddr(0);
  Spinor* temp_ptr = (Spinor*)temp.getaddr(0);
  Spinor* v_ptr   = (Spinor*)v.getaddr(0);
  Spinor* lpf_ptr = (Spinor*)lpf.getaddr(0);
  Spinor* lmf_ptr = (Spinor*)lmf.getaddr(0);
  Spinor* ey_ptr  = (Spinor*)ey.getaddr(0);
  Spinor* w_ptr = (Spinor*)w.getaddr(0);

  int spin_idx;
  double cs, bs;
  double minus_kappa = -Dw_.getKappa();
  double* pU       = const_cast<Field *>(u_)->getaddr(0);


  int tid, nid;
  int is, ns;


  ///////////////////////////////  doe
#pragma omp parallel private(tid,nid,is,ns) firstprivate(temp_ptr)
  {
    nid = omp_get_num_threads();
    tid = omp_get_thread_num();
    is = (Nvol * tid / nid);
    ns = Nvol/nid;
    
    // 5d hopping term
    BGWilsonLA_MultScalar(temp_ptr+is, temp_ptr+is, 1.0/ Params.dp_[0], ns);
    
    for(int s=1; s<N5_-1; ++s){
      Spinor* temp_ptr   = (Spinor*)temp.getaddr(s*f4size_);
      BGWilsonLA_Proj_M(lmf_ptr+is,
			(Spinor*)(const_cast<Field&>(temp).getaddr(Dw_.get_fermionFormat().index(0,0,s-1)))+is,
			ns);
      BGWilsonLA_MultAddScalar(temp_ptr+is,     lmf_ptr+is,Params.dm_[s-1],ns);
      BGWilsonLA_MultScalar(temp_ptr+is, temp_ptr+is, 1.0/ Params.dp_[s], ns);   
    } 
    
    temp_ptr   += (N5_-1)*Nvol;
    BGWilsonLA_Proj_M(v_ptr+is,
		      (Spinor*)(const_cast<Field&>(temp).getaddr(Dw_.get_fermionFormat().index(0,0,N5_-2)))+is,ns);   
    BGWilsonLA_MultAddScalar(temp_ptr+is, v_ptr+is,Params.dm_[N5_-2],ns);
    for(int s=0; s<N5_-1; ++s) {
      BGWilsonLA_Proj_P(ey_ptr+is,
			(Spinor*)(const_cast<Field&>(temp).getaddr(Dw_.get_fermionFormat().index(0,0,s)))+is,ns); 
      BGWilsonLA_MultAddScalar(temp_ptr+is, ey_ptr+is,-mq_*Params.fs_[s],ns);
    }
    BGWilsonLA_MultScalar(temp_ptr+is, temp_ptr+is,1.0/(Params.dp_[N5_-1] +mq_*Params.dm_[N5_-2]*Params.es_[N5_-2]),ns);
    
    
    for(int s=N5_-2; s>=0; --s){ 
      Spinor* temp_ptr   = (Spinor*)temp.getaddr(s*f4size_);
      double fact_lpf = (Params.dm_[s+1]/Params.dp_[s]);
      
      BGWilsonLA_Proj_P(lpf_ptr+is,
			(Spinor*)(const_cast<Field&>(temp).getaddr(Dw_.get_fermionFormat().index(0,0,s+1)))+is,ns); 
      BGWilsonLA_Proj_M(ey_ptr+is,
			(Spinor*)(const_cast<Field&>(temp).getaddr(Dw_.get_fermionFormat().index(0,0,N5_-1)))+is,ns);
      BGWilsonLA_AXPBYPZ(temp_ptr+is, lpf_ptr+is,ey_ptr+is, temp_ptr+is,
			 fact_lpf, - mq_*Params.es_[s],ns);
    }

    //mult_off_diag deo
    for(int s=0; s<N5_; ++s){
      spin_idx = s*f4size_;
      Spinor* temp_ptr = (Spinor*)temp.getaddr(spin_idx);
      Spinor* w5_ptr = (Spinor*)w5.getaddr(spin_idx);
      Spinor* v5_ptr = (Spinor*)v5.getaddr(spin_idx);
      
      bs = (4.0+M0_)*Params.bs_[s];
      cs = (4.0+M0_)*Params.cs_[s];
      
      BGWilson_MultEO_Dag(w_ptr, pU, temp_ptr, minus_kappa , 2, BGWILSON_DIRAC);
      
      BGWilsonLA_MultScalar(w5_ptr+is, w_ptr+is, bs,ns);
      BGWilsonLA_MultScalar(v5_ptr+is, w_ptr+is, cs,ns);
    }


    for(int s = 0; s < N5_; ++s){
      spin_idx = s*f4size_;
      Spinor* w5_ptr = (Spinor*)w5.getaddr(spin_idx);
      Spinor* v5_ptr = (Spinor*)v5.getaddr(spin_idx);

      BGWilsonLA_Proj_P(lpf_ptr+is,
			(Spinor*)(const_cast<Field&>(v5).getaddr(Dw_.get_fermionFormat().index(0,0,(s+1)%N5_)))+is,
			ns);
      BGWilsonLA_Proj_M(lmf_ptr+is,
			(Spinor*)(const_cast<Field&>(v5).getaddr(Dw_.get_fermionFormat().index(0,0,(s+N5_-1)%N5_)))+is,
			ns);
    
      if(s == N5_-1){
	BGWilsonLA_AXPBYPZ(w5_ptr+is, lpf_ptr+is,lmf_ptr+is,
			   w5_ptr+is, -mq_,1.0,ns);
      }
      else if(s==0){
	BGWilsonLA_AXPBYPZ(w5_ptr+is, lpf_ptr+is,lmf_ptr+is, w5_ptr+is,
			   1.0,-mq_,ns);
      }
      else{
	BGWilsonLA_AXPBYPZ(w5_ptr+is, lpf_ptr+is,lmf_ptr+is, w5_ptr+is,
			   1.0,1.0,ns);
      }
    }

    // 5d hopping term
    BGWilsonLA_MultScalar(w5_ptr+is, w5_ptr+is, 1.0/ Params.dp_[0], ns);

    for(int s=1; s<N5_-1; ++s){
      w5_ptr   = (Spinor*)w5.getaddr(s*f4size_);
      BGWilsonLA_Proj_M(lmf_ptr+is,
			(Spinor*)(const_cast<Field&>(w5).getaddr(Dw_.get_fermionFormat().index(0,0,s-1)))+is,ns);
      BGWilsonLA_MultAddScalar(w5_ptr+is,     lmf_ptr+is,Params.dm_[s-1],ns);
      BGWilsonLA_MultScalar(w5_ptr+is, w5_ptr+is, 1.0/ Params.dp_[s], ns);   
    } 

    w5_ptr   = (Spinor*)w5.getaddr((N5_-1)*f4size_); 
    BGWilsonLA_Proj_M(v_ptr+is,
		      (Spinor*)(const_cast<Field&>(w5).getaddr(Dw_.get_fermionFormat().index(0,0,N5_-2)))+is,ns);   
    BGWilsonLA_MultAddScalar(w5_ptr+is, v_ptr+is,Params.dm_[N5_-2],ns);
    for(int s=0; s<N5_-1; ++s) {
      BGWilsonLA_Proj_P(ey_ptr+is,
			(Spinor*)(const_cast<Field&>(w5).getaddr(Dw_.get_fermionFormat().index(0,0,s)))+is,ns); 
      BGWilsonLA_MultAddScalar(w5_ptr+is, ey_ptr+is,-mq_*Params.fs_[s],ns);
    }
    BGWilsonLA_MultScalar(w5_ptr+is, w5_ptr+is,1.0/(Params.dp_[N5_-1] +mq_*Params.dm_[N5_-2]*Params.es_[N5_-2]),ns);


    for(int s=N5_-2; s>=0; --s){ 
      Spinor* w5_ptr   = (Spinor*)w5.getaddr(s*f4size_);
      double fact_lpf = (Params.dm_[s+1]/Params.dp_[s]);
    
      BGWilsonLA_Proj_P(lpf_ptr+is,
			(Spinor*)(const_cast<Field&>(w5).getaddr(Dw_.get_fermionFormat().index(0,0,s+1)))+is,ns); 
      BGWilsonLA_Proj_M(ey_ptr+is,
			(Spinor*)(const_cast<Field&>(w5).getaddr(Dw_.get_fermionFormat().index(0,0,N5_-1)))+is,ns);
      BGWilsonLA_AXPBYPZ(w5_ptr+is, lpf_ptr+is,ey_ptr+is, w5_ptr+is,
			 fact_lpf, - mq_*Params.es_[s],ns);
    }
  
    //mult_off_diag doe
    for(int s=0; s<N5_; ++s){
      spin_idx = s*f4size_;
      Spinor* w5_ptr = (Spinor*)w5.getaddr(spin_idx);
      Spinor* v5_ptr = (Spinor*)v5.getaddr(spin_idx);
    
      bs = (4.0+M0_)*Params.bs_[s];
      cs = (4.0+M0_)*Params.cs_[s];
    
      BGWilson_MultEO_Dag(w_ptr, pU, w5_ptr, minus_kappa , 1, BGWILSON_DIRAC);
    
      BGWilsonLA_MultScalar(w5_ptr+is, w_ptr+is, bs,ns);
      BGWilsonLA_MultScalar(v5_ptr+is, w_ptr+is, cs,ns);
    
    }

    for(int s = 0; s < N5_; ++s){
      spin_idx = s*f4size_;
      Spinor* w5_ptr = (Spinor*)w5.getaddr(spin_idx);
      Spinor* v5_ptr = (Spinor*)v5.getaddr(spin_idx);

      BGWilsonLA_Proj_P(lpf_ptr+is,
			(Spinor*)(const_cast<Field&>(v5).getaddr(Dw_.get_fermionFormat().index(0,0,(s+1)%N5_)))+is,
			ns);
      BGWilsonLA_Proj_M(lmf_ptr+is,
			(Spinor*)(const_cast<Field&>(v5).getaddr(Dw_.get_fermionFormat().index(0,0,(s+N5_-1)%N5_)))+is,
			ns);
    
      if(s == N5_-1){
	BGWilsonLA_AXPBYPZ(w5_ptr+is, lpf_ptr+is,lmf_ptr+is,
			   w5_ptr+is, -mq_,1.0,ns);
      }
      else if(s==0){
	BGWilsonLA_AXPBYPZ(w5_ptr+is, lpf_ptr+is,lmf_ptr+is, w5_ptr+is,
			   1.0,-mq_,ns);
      }
      else{
	BGWilsonLA_AXPBYPZ(w5_ptr+is, lpf_ptr+is,lmf_ptr+is, w5_ptr+is,
			   1.0,1.0,ns);
      }
    }

    Spinor* f5_ptr   = (Spinor*)(const_cast<Field&>(f5).getaddr(0));
    Spinor* w5_ptr   = (Spinor*)w5.getaddr(0);
    for(int s=0; s<N5_; ++s) {
      BGWilsonLA_MultScalar_Add(w5_ptr+is,f5_ptr+is, -1.0, ns);
      w5_ptr += Nvol;
      f5_ptr += Nvol;
    }    
  
  }

}
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_THREADING
void Dirac_optimalDomainWall::mult_hop_dag_omp(Field& w5, const void* f5) const
{
	int tid, nid;
	nid = omp_get_num_threads();
	tid = omp_get_thread_num();

	BGQThread_Barrier(0, nid);
	if(tid == 0){
		BGWilson_DW_Init(N5_,(double*)&Params.dp_[0],(double*)&Params.dm_[0],(double*)&Params.bs_[0],(double*)&Params.cs_[0],(double*)&Params.es_[0],(double*)&Params.fs_[0]);
	}
	BGQThread_Barrier(0, nid);

	BGWilson_DW_Mult_hop_dag(w5.getaddr(0),(void*)(const_cast<Field *>(u_)->getaddr(0)),(void*)f5,
								Dw_.getKappa(),mq_,M0_,BGWILSON_DIRAC);

	/*
  //Allocate fields
  int tid, nid;
  nid = omp_get_num_threads();
  tid = omp_get_thread_num();
  BGQThread_Barrier(0, nid);
  void* lpf_ptr  = BGQThread_Malloc(sizeof(double)*f4size_, nid);
  void* lmf_ptr  = BGQThread_Malloc(sizeof(double)*f4size_, nid);
  BGQThread_Barrier(0, nid);

  mult_hop_dag_omp_allocated(w5,f5,lpf_ptr, lmf_ptr, nid,tid);
  

  //Free fields
  BGQThread_Barrier(0, nid);
//  if (tid==0){
//    free(lpf_ptr);
//    free(lmf_ptr);
//  }
	BGQThread_Free(lpf_ptr,tid);
	BGQThread_Free(lmf_ptr,tid);
  BGQThread_Barrier(0, nid);
	*/
}



void Dirac_optimalDomainWall::mult_hop_dag_omp_allocated(Field& w5, 
							 const void* f5,
							 void* lpf_ptr_void,
							 void* lmf_ptr_void,
							 int nid,
							 int tid) const
{

	/*
	BGWilson_DW_Mult_hop_dag(w5.getaddr(0),(void*)(const_cast<Field *>(u_)->getaddr(0)),(void*)f5,
								Dw_.getKappa(),mq_,M0_,BGWILSON_DIRAC);
	*/

  register int Nvol = CommonPrms::instance()->Nvol()/2;

  //threading control variables
  //int tid, nid;
  int is, is5, ns, ns5 , ie, ie5;
  //nid = omp_get_num_threads();
  //tid = omp_get_thread_num();

  is  = (Nvol * tid / nid);
  is5 = (Nvol * N5_* tid / nid);
//  ns  =  Nvol/nid;
//  ns5 = (Nvol * N5_)/nid;
  ie  = (Nvol * (tid+1) / nid);
  ie5 = (Nvol * N5_* (tid+1) / nid);
  ns = ie - is;
  ns5 = ie5-is5;

  // 4d vectors
  //Spinor* lpf_ptr  = (Spinor*)BGQThread_Malloc(sizeof(double)*f4size_, nid);
  Spinor* lpf_ptr  = (Spinor*)lpf_ptr_void;
  //Spinor* lmf_ptr  = (Spinor*)BGQThread_Malloc(sizeof(double)*f4size_, nid);
  Spinor* lmf_ptr  = (Spinor*)lmf_ptr_void;

  // 5d vectors
  Spinor* temp_ptr_base = (Spinor*)BGQThread_Malloc(sizeof(double)*fsize_, nid);

  Spinor* f5_ptr        = (Spinor*)f5;
  Spinor* w5_ptr_base   = (Spinor*)w5.getaddr(0);
  Spinor* temp_ptr      = temp_ptr_base;
  Spinor* w5_ptr;

  memcpy(temp_ptr_base+is5, f5_ptr+is5, sizeof(Spinor)*ns5);

  int spin_idx;
  double cs, bs;
  double minus_kappa = -Dw_.getKappa();
  double* pU = const_cast<Field *>(u_)->getaddr(0);

  //////////////////////////////////// 5d hopping term
  //
  BGWilsonLA_MultScalar(temp_ptr+is, temp_ptr+is, 1.0/ Params.dp_[0], ns);
    
  for(int s=1; s<N5_-1; ++s){
    temp_ptr   = temp_ptr_base+s*Nvol;
    BGWilsonLA_Proj_M(lmf_ptr+is,
		      temp_ptr_base+(s-1)*Nvol+is,
		      ns);
    BGWilsonLA_MultAddScalar(temp_ptr+is,     lmf_ptr+is,Params.dm_[s-1],ns);
    BGWilsonLA_MultScalar(temp_ptr+is, temp_ptr+is, 1.0/ Params.dp_[s], ns);   
  } 
    
  // temp_ptr   += (N5_-1)*Nvol;
  temp_ptr   += Nvol;
  BGWilsonLA_Proj_M(lpf_ptr+is,
		    temp_ptr_base+(N5_-2)*Nvol+is,
		    ns);   
  BGWilsonLA_MultAddScalar(temp_ptr+is, lpf_ptr+is,Params.dm_[N5_-2],ns);

  for(int s=0; s<N5_-1; ++s) {
    BGWilsonLA_Proj_P(lpf_ptr+is,
		      temp_ptr_base+(s)*Nvol+is,
		      ns); 
    BGWilsonLA_MultAddScalar(temp_ptr+is, lpf_ptr+is,-mq_*Params.fs_[s],ns);
  }

  BGWilsonLA_MultScalar(temp_ptr+is, 
			temp_ptr+is,
			1.0/(Params.dp_[N5_-1] +mq_*Params.dm_[N5_-2]*Params.es_[N5_-2]),
			ns);
    
  for(int s=N5_-2; s>=0; --s){ 
    Spinor* temp_ptr   = temp_ptr_base+s*Nvol;
    double fact_lpf = (Params.dm_[s+1]/Params.dp_[s]);
      
    BGWilsonLA_Proj_P(lpf_ptr+is,
		      temp_ptr_base+(s+1)*Nvol+is,
		      ns); 
    BGWilsonLA_Proj_M(lmf_ptr+is,
		      temp_ptr_base+(N5_-1)*Nvol+is,
		      ns);
    BGWilsonLA_AXPBYPZ(temp_ptr+is, lpf_ptr+is,lmf_ptr+is, temp_ptr+is,
		       fact_lpf, - mq_*Params.es_[s],ns);
  }



  //mult_off_diag deo
  for(int s=0; s<N5_; ++s){
    spin_idx = s*f4size_;
    Spinor* temp_ptr = temp_ptr_base+s*Nvol;
    Spinor* w5_ptr = (Spinor*)w5.getaddr(spin_idx);
      
    bs = (4.0+M0_)*Params.bs_[s];
    cs = (4.0+M0_)*Params.cs_[s];
      
    BGWilson_MultEO_Dag(lpf_ptr, pU, temp_ptr, minus_kappa , 2, BGWILSON_DIRAC);
      
    BGWilsonLA_MultScalar(w5_ptr+is, lpf_ptr+is, bs,ns);
    BGWilsonLA_MultScalar(temp_ptr+is, lpf_ptr+is, cs,ns);
  }


  for(int s = 0; s < N5_; ++s){
    spin_idx = s*f4size_;
    Spinor* w5_ptr = (Spinor*)w5.getaddr(spin_idx);

    BGWilsonLA_Proj_P(lpf_ptr+is,
		      temp_ptr_base+((s+1)%N5_)*Nvol+is,
		      ns);
    BGWilsonLA_Proj_M(lmf_ptr+is,
		      temp_ptr_base+((s+N5_-1)%N5_)*Nvol+is,
		      ns);


    if(s == N5_-1){
      BGWilsonLA_AXPBYPZ(w5_ptr+is, lpf_ptr+is,lmf_ptr+is,
			 w5_ptr+is, -mq_,1.0,ns);
    }
    else if(s==0){
      BGWilsonLA_AXPBYPZ(w5_ptr+is, lpf_ptr+is,lmf_ptr+is, w5_ptr+is,
			 1.0,-mq_,ns);
    }
    else{
      BGWilsonLA_AXPBYPZ(w5_ptr+is, lpf_ptr+is,lmf_ptr+is, w5_ptr+is,
			 1.0,1.0,ns);
    }
  }

  /////////////////////////////////// 5d hopping term
  /////////
  BGWilsonLA_MultScalar(w5_ptr_base+is, w5_ptr_base+is, 1.0/ Params.dp_[0], ns);

  for(int s=1; s<N5_-1; ++s){
    w5_ptr   = w5_ptr_base + s*Nvol;
    BGWilsonLA_Proj_M(lmf_ptr+is,
		      w5_ptr_base+Nvol*(s-1)+is,
		      ns);
    BGWilsonLA_MultAddScalar(w5_ptr+is,     lmf_ptr+is,Params.dm_[s-1],ns);
    BGWilsonLA_MultScalar(w5_ptr+is, w5_ptr+is, 1.0/ Params.dp_[s], ns);   
  } 

  w5_ptr   = (Spinor*)w5.getaddr((N5_-1)*f4size_); 

  BGWilsonLA_Proj_M(lpf_ptr+is,
		    w5_ptr_base+Nvol*(N5_-2)+is,
		    ns);   
  BGWilsonLA_MultAddScalar(w5_ptr+is, lpf_ptr+is,Params.dm_[N5_-2],ns);

  for(int s=0; s<N5_-1; ++s) {
    BGWilsonLA_Proj_P(lpf_ptr+is,
		      w5_ptr_base+Nvol*s+is,
		      ns); 
    BGWilsonLA_MultAddScalar(w5_ptr+is, lpf_ptr+is,-mq_*Params.fs_[s],ns);
  }

  BGWilsonLA_MultScalar(w5_ptr+is, 
			w5_ptr+is,
			1.0/(Params.dp_[N5_-1] +mq_*Params.dm_[N5_-2]*Params.es_[N5_-2]),
			ns);

  for(int s=N5_-2; s>=0; --s){ 
    Spinor* w5_ptr   = (Spinor*)w5.getaddr(s*f4size_);
    double fact_lpf = (Params.dm_[s+1]/Params.dp_[s]);
    
    BGWilsonLA_Proj_P(lpf_ptr+is,
		      w5_ptr_base+Nvol*(s+1)+is,
		      ns); 
    BGWilsonLA_Proj_M(lmf_ptr+is,
		      w5_ptr_base+Nvol*(N5_-1)+is,
		      ns);
    BGWilsonLA_AXPBYPZ(w5_ptr  + is, 
		       lpf_ptr + is,
		       lmf_ptr  + is, 
		       w5_ptr  + is,
		       fact_lpf, - mq_*Params.es_[s],ns);
  }


  /////////////////////////// mult_off_diag doe
  for(int s=0; s<N5_; ++s){
    spin_idx = s*Nvol;
    Spinor* w5_ptr = w5_ptr_base + spin_idx;
    Spinor* temp_ptr = temp_ptr_base + spin_idx;
    
    bs = (4.0+M0_)*Params.bs_[s];
    cs = (4.0+M0_)*Params.cs_[s];
    
    BGWilson_MultEO_Dag(lpf_ptr, pU, w5_ptr, minus_kappa , 1, BGWILSON_DIRAC);
    
    BGWilsonLA_MultScalar(w5_ptr+is, lpf_ptr+is, bs,ns);
    BGWilsonLA_MultScalar(temp_ptr+is, lpf_ptr+is, cs,ns);
    
  }

  for(int s = 0; s < N5_; ++s){
    spin_idx = s*f4size_;
    Spinor* w5_ptr = (Spinor*)w5.getaddr(spin_idx);
    Spinor* temp_ptr = temp_ptr_base+s*Nvol;

    BGWilsonLA_Proj_P(lpf_ptr+is,
		      temp_ptr_base+((s+1)%N5_)*Nvol+is,
		      ns);
    BGWilsonLA_Proj_M(lmf_ptr+is,
		      temp_ptr_base+((s+N5_-1)%N5_)*Nvol+is,
		      ns);
    
    if(s == N5_-1){
      BGWilsonLA_AXPBYPZ(w5_ptr+is, lpf_ptr+is,lmf_ptr+is,
			 w5_ptr+is, -mq_,1.0,ns);
    }
    else if(s==0){
      BGWilsonLA_AXPBYPZ(w5_ptr+is, lpf_ptr+is,lmf_ptr+is, w5_ptr+is,
			 1.0,-mq_,ns);
    }
    else{
      BGWilsonLA_AXPBYPZ(w5_ptr+is, lpf_ptr+is,lmf_ptr+is, w5_ptr+is,
			 1.0,1.0,ns);
    }
  }


  w5_ptr   = w5_ptr_base;
  for(int s=0; s<N5_; ++s) {
    BGWilsonLA_MultScalar_Add(w5_ptr+is,f5_ptr+is, -1.0, ns);
    w5_ptr += Nvol;
    f5_ptr += Nvol;
  } 
   
 BGQThread_Barrier(0,nid);  
 //#pragma omp single
// if(tid==0)
//  {  
    //free(lpf_ptr);
    //free(lmf_ptr);
//  free(temp_ptr_base);
//  }
  BGQThread_Free(temp_ptr_base,tid);
  BGQThread_Barrier(0,nid);

}
#endif



#ifdef IBM_BGQ_WILSON


// CG solver optimized for BGQ
void Dirac_optimalDomainWall::solve_eo_5d(Field& w5, 
					  const Field& b, 
					  SolverOutput& Out, 
					  int MaxIter, 
					  double GoalPrecision) const
{
  
  #if VERBOSITY>=SOLV_ITER_VERB_LEVEL
  CCIO::header("CG_BGQ solver start");
  #endif

#ifdef ENABLE_THREADING


  BGQThread_Init(); //initializing BGQ fast threading routines


  Out.Msg = "CG solver";
  Out.Iterations = -1;
  double kernel_timing = 0.0; 
  int tid, nid;
  int is, ie, ns;

  register int Nvol = CommonPrms::instance()->Nvol()/2;
  Field temp(fsize_), temp2(fsize_), s(fsize_); //eventually eliminated
  Field p(fsize_);
  int v_size = Nvol*N5_; // << assumes 24 elements

//  double threadNorm[64];
  double pap, rrp, cr, rr, snorm;

  TIMING_START;
  
  Field x = b;//initial condition
  Field r = b;//initial residual

  Spinor* x_ptr = (Spinor*)x.getaddr(0);
  Spinor* r_ptr = (Spinor*)r.getaddr(0);
  Spinor* s_ptr = (Spinor*)s.getaddr(0);
  Spinor* w5_ptr = (Spinor*)w5.getaddr(0);
  Spinor* b_ptr = (Spinor*)const_cast<Field&>(b).getaddr(0);
  Spinor* temp_ptr = (Spinor*)temp.getaddr(0);
  Spinor* temp2_ptr = (Spinor*)temp2.getaddr(0);
  Spinor* p_ptr = (Spinor*)p.getaddr(0);

//  void* aux1_ptr  = malloc(sizeof(double)*f4size_);
//  void* aux2_ptr  = malloc(sizeof(double)*f4size_);
  //void* aux3_ptr  = malloc(sizeof(double)*fsize_);

 	BGWilson_DW_Init(N5_,(double*)&Params.dp_[0],(double*)&Params.dm_[0],(double*)&Params.bs_[0],(double*)&Params.cs_[0],(double*)&Params.es_[0],(double*)&Params.fs_[0]);

#pragma omp parallel 
	{
		int tid, nid;
		int is, ie, ns,s5;
		double pap, rrp, cr, rr, snorm;
		double tSum,t;

		nid = omp_get_num_threads();
		tid = omp_get_thread_num();

		is = tid*Nvol / nid;
		ie = (tid + 1)*Nvol / nid;
		ns = ie - is;

		BGWilson_DW_Mult_hop(temp_ptr,(void*)(const_cast<Field *>(u_)->getaddr(0)),x_ptr,
								Dw_.getKappa(),mq_,M0_,BGWILSON_DIRAC);
		BGWilson_DW_Mult_hop_dag(temp2_ptr,(void*)(const_cast<Field *>(u_)->getaddr(0)),temp_ptr,
								Dw_.getKappa(),mq_,M0_,BGWILSON_DIRAC);

		/*
		mult_hop_omp_allocated(temp,x_ptr, aux1_ptr, aux2_ptr, nid, tid);
		mult_hop_dag_omp_allocated(temp2,temp_ptr, aux1_ptr, aux2_ptr,nid, tid);
		*/

		for(s5=0;s5<N5_;s5++)
			BGWilsonLA_Sub(r_ptr+s5*Nvol+is, temp2_ptr+s5*Nvol+is, ns);

		for(s5=0;s5<N5_;s5++)
			BGWilsonLA_Equate(p_ptr+s5*Nvol+is, r_ptr+s5*Nvol+is, ns);

		tSum = 0.0;
		for(s5=0;s5<N5_;s5++){
			BGWilsonLA_Norm(&t,r_ptr+s5*Nvol+is,ns);
			tSum += t;
		}

		tSum = BGQThread_GatherDouble(tSum,0,tid,nid);
		if(tid == 0){
			tSum = Communicator::instance()->reduce_sum(tSum);
		}
		rr = BGQThread_ScatterDouble(tSum,0,tid,nid);

		tSum = 0.0;
		for(s5=0;s5<N5_;s5++){
			BGWilsonLA_Norm(&t,b_ptr+s5*Nvol+is,ns);
			tSum += t;
		}
		tSum = BGQThread_GatherDouble(tSum,0,tid,nid);
		if(tid == 0){
			tSum = Communicator::instance()->reduce_sum(tSum);
		}
		snorm = BGQThread_ScatterDouble(tSum,0,tid,nid);
		snorm = sqrt(snorm);
		snorm = 1.0/snorm;

#if VERBOSITY>1
#pragma omp single
    {
      CCIO::cout<<" Snorm = "<< snorm << std::endl;
      CCIO::cout<<" Init  = "<< rr*snorm<< std::endl;
    }
#endif

		for(int it = 0; it < MaxIter; ++it){
			double tPAP;
			BGWilson_DW_Mult_hop(temp_ptr,(void*)(const_cast<Field *>(u_)->getaddr(0)),p_ptr,
								Dw_.getKappa(),mq_,M0_,BGWILSON_DIRAC);
			BGWilson_DW_Mult_hop_dag(s_ptr,(void*)(const_cast<Field *>(u_)->getaddr(0)),temp_ptr,
								Dw_.getKappa(),mq_,M0_,BGWILSON_DIRAC);

			/*
			//      mult_hop_omp(temp,p_ptr);
			mult_hop_omp_allocated(temp,p_ptr, aux1_ptr, aux2_ptr, nid, tid);
			//mult_hop_dag_omp(s,temp_ptr);
			mult_hop_dag_omp_allocated(s,temp_ptr, aux1_ptr, aux2_ptr,nid, tid);
			*/

			///////////////////////////////////////////////////
			tSum = 0.0;
			for(s5=0;s5<N5_;s5++){
				BGWilsonLA_DotProd(&t,p_ptr+s5*Nvol+is,s_ptr+s5*Nvol+is,ns);
				tSum += t;
			}
			tSum = BGQThread_GatherDouble(tSum,0,tid,nid);
			if(tid == 0){
				tSum = Communicator::instance()->reduce_sum(tSum);
			}
			pap = BGQThread_ScatterDouble(tSum,0,tid,nid);
			rrp = rr;
			cr = rrp/pap;// (r,r)/(p,Ap)

			//  x += cr*p; // x = x + cr * p
			for(s5=0;s5<N5_;s5++)
				BGWilsonLA_MultAddScalar(x_ptr+s5*Nvol+is,p_ptr+s5*Nvol+is,cr,ns);
			//  r -= cr*s; // r_k = r_k - cr * Ap
			for(s5=0;s5<N5_;s5++)
				BGWilsonLA_MultAddScalar(r_ptr+s5*Nvol+is,s_ptr+s5*Nvol+is,-cr,ns);
			//  rr = r*r; // rr = (r_k,r_k)
			tSum = 0.0;
			for(s5=0;s5<N5_;s5++){
				BGWilsonLA_Norm(&t,r_ptr+s5*Nvol+is,ns);
				tSum += t;
			}

			tSum = BGQThread_GatherDouble(tSum,0,tid,nid);
			if(tid == 0){
				tSum = Communicator::instance()->reduce_sum(tSum);
			}
			rr = BGQThread_ScatterDouble(tSum,0,tid,nid);

			//  p *= rr/rrp; // p = p*(r_k,r_k)/(r,r)
			//  p += r; // p = p + p*(r_k,r_k)/(r,r)
			for(s5=0;s5<N5_;s5++)
				BGWilsonLA_MultScalar_Add(p_ptr+s5*Nvol+is,r_ptr+s5*Nvol+is,rr/rrp,ns);

			//BGQThread_Barrier(0,nid);

#if VERBOSITY>1
			if (tid==0) {
				CCIO::cout<< std::setw(5)<< "["<<it<<"] "
				<< std::setw(20) << rr*snorm<<"\n";
			} 
#endif
			BGQThread_Barrier(0,nid);
			if(rr*snorm < GoalPrecision){
				if (tid==0) Out.Iterations = it;
				break;
			}
		}//end of iterations loop      

#pragma omp single
		{
			if(Out.Iterations == -1) {
				CCIO::cout<<" Not converged. Current residual: "<< rr*snorm << "\n";
				abort();
			}
		}
		BGQThread_Barrier(0,nid);//after loop
		//p = opr_->mult(x);

		BGWilson_DW_Mult_hop(temp_ptr,(void*)(const_cast<Field *>(u_)->getaddr(0)),x_ptr,
							Dw_.getKappa(),mq_,M0_,BGWILSON_DIRAC);
		BGWilson_DW_Mult_hop_dag(p_ptr,(void*)(const_cast<Field *>(u_)->getaddr(0)),temp_ptr,
							Dw_.getKappa(),mq_,M0_,BGWILSON_DIRAC);
		/*
		mult_hop_omp_allocated(temp,x_ptr, aux1_ptr, aux2_ptr, nid, tid);
		mult_hop_dag_omp_allocated(p,temp_ptr, aux1_ptr, aux2_ptr, nid, tid);
		*/

		//  p -= b;
		for(s5=0;s5<N5_;s5++)
			BGWilsonLA_Sub(p_ptr+s5*Nvol+is, b_ptr+s5*Nvol+is, ns);

		//Out.diff = p.norm();  
		tSum = 0.0;
		for(s5=0;s5<N5_;s5++){
			BGWilsonLA_Norm(&t,p_ptr+s5*Nvol+is,ns);
			tSum += t;
		}

		tSum = BGQThread_GatherDouble(tSum,0,tid,nid);
		if(tid == 0){
			tSum = Communicator::instance()->reduce_sum(tSum);
			Out.diff = sqrt(tSum);
		}
		rr = BGQThread_ScatterDouble(tSum,0,tid,nid);

		//w5 = x;  
		for(s5=0;s5<N5_;s5++)
			BGWilsonLA_Equate(w5_ptr+s5*Nvol+is, x_ptr+s5*Nvol+is, ns);
	}

  TIMING_END(Out.timing);
  CCIO::cout << "Kernel section timing: "<< kernel_timing << "\n";

//  free(aux1_ptr);
//  free(aux2_ptr);
  //free(aux3_ptr);
#endif

}




#endif
