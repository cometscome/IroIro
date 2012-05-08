//-----------------------------------------------------------------
// communicator_mpi.cpp
//-----------------------------------------------------------------
#include "communicator.h"
#include "commonPrms.h"

#ifndef IBM_BGQ
#include "mpi.h"
#else
#include "bgnet.h"
#endif

#include "comm_io.hpp"
#include <stdio.h>
#include <iostream>
#include <cstdarg>
using namespace std;

int Communicator::my_rank_;
int Communicator::Nproc_;
int Communicator::ipe_[Ndim_max_];
int Communicator::nd_up_[Ndim_max_];
int Communicator::nd_dn_[Ndim_max_];

#ifndef IBM_BGQ

Communicator* Communicator::instance(){
  static Communicator instance_;
  return &instance_;
}

void Communicator::setup(){
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank_);
  MPI_Comm_size(MPI_COMM_WORLD, &Nproc_);

  int nodeid = my_rank_;

  int NPEx = CommonPrms::instance()->NPEx();
  int NPEy = CommonPrms::instance()->NPEy();
  int NPEz = CommonPrms::instance()->NPEz();
  int NPEt = CommonPrms::instance()->NPEt();

  CCIO::cout.init(&std::cout);
  CCIO::cerr.init(&std::cerr);

  //Check number of nodes
  if (NPEx*NPEy*NPEz*NPEt != Nproc_) {
    if(my_rank_==0) 
      cerr << "Number of nodes provided is different from MPI environment ["
	   << Nproc_<<"]\n"; 
    abort();
  }

  int px = nodeid %NPEx;
  int py = (nodeid/NPEx) %NPEy;
  int pz = (nodeid/(NPEx*NPEy)) %NPEz;
  int pt = nodeid/(NPEx*NPEy*NPEz);

  ipe_[0] = px;
  ipe_[1] = py;
  ipe_[2] = pz;
  ipe_[3] = pt;

  nd_up_[0] = ((px+1)%NPEx) +py*NPEx +pz*NPEx*NPEy +pt*NPEx*NPEy*NPEz;
  nd_up_[1] = px +((py+1)%NPEy)*NPEx +pz*NPEx*NPEy +pt*NPEx*NPEy*NPEz;
  nd_up_[2] = px +py*NPEx +((pz+1)%NPEz)*NPEx*NPEy +pt*NPEx*NPEy*NPEz;
  nd_up_[3] = px +py*NPEx +pz*NPEx*NPEy +((pt+1)%NPEt)*NPEx*NPEy*NPEz;

  nd_dn_[0] = ((px-1+NPEx)%NPEx) +py*NPEx +pz*NPEx*NPEy +pt*NPEx*NPEy*NPEz;
  nd_dn_[1] = px +((py-1+NPEy)%NPEy)*NPEx +pz*NPEx*NPEy +pt*NPEx*NPEy*NPEz;
  nd_dn_[2] = px +py*NPEx +((pz-1+NPEz)%NPEz)*NPEx*NPEy +pt*NPEx*NPEy*NPEz;
  nd_dn_[3] = px +py*NPEx +pz*NPEx*NPEy +((pt-1+NPEt)%NPEt)*NPEx*NPEy*NPEz;

  if(my_rank_==0) cout << "Communicator initialized using MPI with "
		       << Nproc_ << " processes.\n";
}

Communicator::~Communicator(){  MPI_Finalize();}

bool Communicator::primaryNode(){
  if (my_rank_==0) return true;
  return false;
}

//bool Communicator::primaryNode(){ return (my_rank_? true : false);}

int Communicator::nodeid(int x, int y, int z, int t)const{
  int NPEx = CommonPrms::instance()->NPEx();
  int NPEy = CommonPrms::instance()->NPEy();
  int NPEz = CommonPrms::instance()->NPEz();
  int NPEt = CommonPrms::instance()->NPEt();
  return x +NPEx*(y +NPEy*(z +NPEz*t));
}

void Communicator::
transfer_fw(double *bin,double *data,int size,int dir)const{
  MPI_Status  status;
  int p_send = nd_dn_[dir];
  int p_recv = nd_up_[dir];

  int tag1 = dir*Nproc_+my_rank_;
  int tag2 = dir*Nproc_+p_recv;

  MPI_Sendrecv(data, size, MPI_DOUBLE, p_send, tag1,
	       bin,  size, MPI_DOUBLE, p_recv, tag2,
	       MPI_COMM_WORLD, &status );
}

void Communicator::
transfer_fw(valarray<double>& bin,const valarray<double>& data,int dir)const{
  transfer_fw(&bin[0],&(const_cast<valarray<double>& >(data))[0],
	      bin.size(),dir);
}

void Communicator::
transfer_fw(valarray<double>& bin,const valarray<double>& data,
	    const vector<int>& index, int dir)const{
  MPI_Status  status;
  MPI_Datatype subarray;
  MPI_Type_create_indexed_block(index.size(),1,
				&(const_cast<vector<int>& >(index))[0],
				MPI_DOUBLE,&subarray); 
  MPI_Type_commit(&subarray);

  int p_send = nd_dn_[dir];
  int p_recv = nd_up_[dir];
  int tag1 = dir*Nproc_+my_rank_;
  int tag2 = dir*Nproc_+p_recv;

  MPI_Sendrecv(&(const_cast<valarray<double>& >(data))[0],
	       1,subarray,p_send,tag1,
	       &bin[0], index.size(),MPI_DOUBLE,p_recv,tag2,
	       MPI_COMM_WORLD,&status);

  MPI_Type_free (&subarray);
}

void Communicator::
transfer_bk(double *bin,double *data,int size,int dir)const{
  MPI_Status  status;
  int p_send = nd_up_[dir];
  int p_recv = nd_dn_[dir];
  int Ndim = CommonPrms::instance()->Ndim();
  int tag1 = (Ndim +dir)*Nproc_+my_rank_;
  int tag2 = (Ndim +dir)*Nproc_+p_recv;

  MPI_Sendrecv(data, size, MPI_DOUBLE, p_send, tag1,
	       bin,  size, MPI_DOUBLE, p_recv, tag2,
	       MPI_COMM_WORLD, &status );
}

void Communicator::
transfer_bk(valarray<double>& bin,const valarray<double>& data,int dir)const{
  transfer_bk(&(bin[0]),&(const_cast<valarray<double>& >(data))[0],
	      bin.size(),dir);
}

void Communicator::
transfer_bk(valarray<double>& bin,const valarray<double>& data,
	    const vector<int>& index, int dir)const{
  MPI_Status  status;
  MPI_Datatype subarray;
  int MPIErr;
  MPIErr = MPI_Type_create_indexed_block(index.size(),1,
     				&(const_cast<vector<int>& >(index))[0],
     				MPI_DOUBLE,&subarray); 
  MPI_Type_commit(&subarray);

  int p_send = nd_up_[dir];
  int p_recv = nd_dn_[dir];
  int Ndim = CommonPrms::instance()->Ndim();
  int tag1 = (Ndim +dir)*Nproc_+my_rank_;
  int tag2 = (Ndim +dir)*Nproc_+p_recv;

  MPI_Sendrecv(&(const_cast<valarray<double>& >(data))[0],
	       1,subarray,p_send,tag1,
	       &bin[0], index.size(),MPI_DOUBLE,p_recv,tag2,
	       MPI_COMM_WORLD,&status);

  MPI_Type_free (&subarray);
}

void Communicator::send_1to1(double *bin,double *data,int size,
			     int p_to,int p_from,int tag)const{
  MPI_Status status;
  if(p_to == p_from){
    for(int i=0; i<size; ++i) bin[i] = data[i];
  }else{
   if( my_rank_ == p_from)
     MPI_Send(data, size, MPI_DOUBLE, p_to, tag, MPI_COMM_WORLD );
   if( my_rank_ == p_to)
     MPI_Recv( bin, size, MPI_DOUBLE, p_from, tag, MPI_COMM_WORLD, &status );
  }
  MPI_Barrier(MPI_COMM_WORLD);
}

void Communicator::send_1to1(valarray<double>& bin,
			     const valarray<double>& data, 
			     int size,int p_to,int p_from,int tag)const{
  if(p_to == p_from)    bin = data;
  else send_1to1(&(bin[0]),&(const_cast<valarray<double>& >(data))[0],
		 size,p_to,p_from,tag);
  MPI_Barrier(MPI_COMM_WORLD);
}

double Communicator::reduce_sum(double a)const{
 double a_sum = 0.0;
 MPI_Allreduce(&a, &a_sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
 return a_sum;
}

void Communicator::sync()const{ MPI_Barrier(MPI_COMM_WORLD);}

void Communicator::broadcast(int size, int &data, int sender)const{
 MPI_Bcast( &data, size, MPI_INT, sender, MPI_COMM_WORLD );
}

void Communicator::broadcast(int size, double &data, int sender)const{
  MPI_Bcast( &data, size, MPI_DOUBLE, sender, MPI_COMM_WORLD );
}

int Communicator::reduce_max(double& val,int& idx,int size)const{
  /*! size is the maximum value of idx */
  VaId vi = {val, my_rank_*size +idx};
  VaId vo;
  MPI_Reduce(&vi,&vo,1,MPI_DOUBLE_INT,MPI_MAXLOC,0,MPI_COMM_WORLD);
  val = vo.value;
  idx = vo.index%size;
  return vo.index/size;
}

int Communicator::reduce_min(double& val,int& idx,int size)const{
  /*! size is the maximum value of idx */
  
  VaId vi = {val, my_rank_*size +idx};
  VaId vo;
  MPI_Reduce(&vi,&vo,1,MPI_DOUBLE_INT,MPI_MINLOC,0,MPI_COMM_WORLD);
  val = vo.value;
  idx = vo.index%size;
  return vo.index/size;
}

int Communicator::pprintf(const char* format ...)const{
  va_list ap;
  int ret = 0;

  va_start(ap,format);
  if(my_rank_==0)
    ret = vfprintf( stdout, format, ap );
  va_end(ap);

  return ret;
}


#endif


#ifdef IBM_BGQ

static double* pTmpBuffer = NULL;
static int TmpBufferSize = 0;

Communicator* Communicator::instance(){
  static Communicator instance_;
  return &instance_;
}

void Communicator::setup(){
  BGNET_Init();
  my_rank_ = BGNET_Rank();
  Nproc_ = BGNET_Procs();

  int nodeid = my_rank_;

  int NPEx = CommonPrms::instance()->NPEx();
  int NPEy = CommonPrms::instance()->NPEy();
  int NPEz = CommonPrms::instance()->NPEz();
  int NPEt = CommonPrms::instance()->NPEt();

  //Check number of nodes
  if (NPEx*NPEy*NPEz*NPEt != Nproc_) {
    if(my_rank_==0) 
      cerr << "Number of nodes provided is different from MPI environment ["
	   << Nproc_<<"]\n"; 
    abort();
  }

  int px =  nodeid %NPEx;
  int py = (nodeid/NPEx) %NPEy;
  int pz = (nodeid/(NPEx*NPEy)) %NPEz;
  int pt = (nodeid/(NPEx*NPEy*NPEz)) %NPEt;

  ipe_[0] = px;
  ipe_[1] = py;
  ipe_[2] = pz;
  ipe_[3] = pt;

  nd_up_[0] = ((px+1)%NPEx) +py*NPEx +pz*NPEx*NPEy +pt*NPEx*NPEy*NPEz;
  nd_up_[1] = px +((py+1)%NPEy)*NPEx +pz*NPEx*NPEy +pt*NPEx*NPEy*NPEz;
  nd_up_[2] = px +py*NPEx +((pz+1)%NPEz)*NPEx*NPEy +pt*NPEx*NPEy*NPEz;
  nd_up_[3] = px +py*NPEx +pz*NPEx*NPEy +((pt+1)%NPEt)*NPEx*NPEy*NPEz;

  nd_dn_[0] = ((px-1+NPEx)%NPEx) +py*NPEx +pz*NPEx*NPEy +pt*NPEx*NPEy*NPEz;
  nd_dn_[1] = px +((py-1+NPEy)%NPEy)*NPEx +pz*NPEx*NPEy +pt*NPEx*NPEy*NPEz;
  nd_dn_[2] = px +py*NPEx +((pz-1+NPEz)%NPEz)*NPEx*NPEy +pt*NPEx*NPEy*NPEz;
  nd_dn_[3] = px +py*NPEx +pz*NPEx*NPEy +((pt-1+NPEt)%NPEt)*NPEx*NPEy*NPEz;

  if(my_rank_==0) cout << "Communicator initialized using BGNET with "
		       << Nproc_ << " processes.\n";

  BGNET_GlobalBarrier();
  CCIO::cout.init(&std::cout);
  CCIO::cerr.init(&std::cerr); 
}

Communicator::~Communicator(){  BGNET_Finalize();}

bool Communicator::primaryNode(){
  if (my_rank_==0) return true;
  return false;
}

//bool Communicator::primaryNode(){ return (my_rank_? true : false);}

int Communicator::nodeid(int x, int y, int z, int t)const{
  int NPEx = CommonPrms::instance()->NPEx();
  int NPEy = CommonPrms::instance()->NPEy();
  int NPEz = CommonPrms::instance()->NPEz();
  int NPEt = CommonPrms::instance()->NPEt();
  return x +NPEx*(y +NPEy*(z +NPEz*t));
}

void Communicator::
transfer_fw(double *bin,double *data,int size,int dir) const{
  int p_send = nd_dn_[dir];
  int p_recv = nd_up_[dir];

  int tag1 = dir*Nproc_+my_rank_;
  int tag2 = dir*Nproc_+p_recv;

  BGNET_Sendrecv(0,data,size*sizeof(double),p_send,bin,size*sizeof(double),p_recv);
}

void Communicator::
transfer_fw(valarray<double>& bin,const valarray<double>& data,int dir) const{
  transfer_fw(&bin[0],&(const_cast<valarray<double>& >(data))[0],
	      bin.size(),dir);
}

void Communicator::
transfer_fw(valarray<double>& bin,const valarray<double>& data,
	    const vector<int>& index, int dir) const
{
  double* pTmp;
  double* pIn;
  int i,size;
  int p_send = nd_dn_[dir];
  int p_recv = nd_up_[dir];
  int tag1 = dir*Nproc_+my_rank_;
  int tag2 = dir*Nproc_+p_recv;

  size = index.size();

  if(size > TmpBufferSize){
    if(pTmpBuffer != NULL){
      free(pTmpBuffer);
    }
    pTmpBuffer = (double*)malloc(size*sizeof(double));
    TmpBufferSize = size;
  }

  pIn = &(const_cast<valarray<double>& >(data))[0];
  pTmp = pTmpBuffer;
  for(i=0;i<size;i++){
    pTmp[i] = pIn[index[i]];
  }

  BGNET_Sendrecv(0,pTmp,size*sizeof(double),p_send,&(const_cast<valarray<double>& >(bin))[0],size*sizeof(double),p_recv);
}

void Communicator::
transfer_bk(double *bin,double *data,int size,int dir) const{
  int p_send = nd_up_[dir];
  int p_recv = nd_dn_[dir];
  int Ndim = CommonPrms::instance()->Ndim();
  int tag1 = (Ndim +dir)*Nproc_+my_rank_;
  int tag2 = (Ndim +dir)*Nproc_+p_recv;

  BGNET_Sendrecv(0,data,size*sizeof(double),p_send,bin,size*sizeof(double),p_recv);
}

void Communicator::
transfer_bk(valarray<double>& bin,const valarray<double>& data,int dir) const{
  transfer_bk(&(bin[0]),&(const_cast<valarray<double>& >(data))[0],
	      bin.size(),dir);
}

void Communicator::
transfer_bk(valarray<double>& bin,const valarray<double>& data,
	    const vector<int>& index, int dir) const
{
  double* pTmp;
  double* pIn;
  int i,size;
  int p_send = nd_up_[dir];
  int p_recv = nd_dn_[dir];

  size = index.size();

  if(size > TmpBufferSize){
    if(pTmpBuffer != NULL){
      free(pTmpBuffer);
    }
    pTmpBuffer = (double*)malloc(size*sizeof(double));
    TmpBufferSize = size;
  }

  pIn = &(const_cast<valarray<double>& >(data))[0];
  pTmp = pTmpBuffer;
  for(i=0;i<size;i++){
    pTmp[i] = pIn[index[i]];
  }

  BGNET_Sendrecv(0,pTmp,size*sizeof(double),p_send,&(const_cast<valarray<double>& >(bin))[0],size*sizeof(double),p_recv);
}

void Communicator::send_1to1(double *bin,double *data,int size,
			     int p_to,int p_from,int tag) const{
  if(p_to == p_from){
    for(int i=0; i<size; ++i) bin[i] = data[i];
  }else{
    if( my_rank_ == p_from)
      BGNET_Send(0,data,size*sizeof(double),p_to);
    if( my_rank_ == p_to)
      BGNET_Recv(0,bin,size*sizeof(double),p_from);
  }
  BGNET_GlobalBarrier();
}

void Communicator::send_1to1(valarray<double>& bin,
			     const valarray<double>& data, 
			     int size,int p_to,int p_from,int tag) const{
  if(p_to == p_from)    bin = data;
  else send_1to1(&(bin[0]),&(const_cast<valarray<double>& >(data))[0],
		 size,p_to,p_from,tag);
  BGNET_GlobalBarrier();
}

double Communicator::reduce_sum(double a) const{
  double a_sum = 0.0;
  BGNET_GlobalSum(&a_sum,&a);
  return a_sum;
}

void Communicator::sync() const{ BGNET_GlobalBarrier();}

void Communicator::broadcast(int size, int &data, int sender) const{
  BGNET_BCast(&data,size,BGNET_COLLECTIVE_INT32,sender,BGNET_COMM_WORLD);
}

void Communicator::broadcast(int size, double &data, int sender) const{
  BGNET_BCast(&data,size,BGNET_COLLECTIVE_DOUBLE,sender,BGNET_COMM_WORLD);
}

int Communicator::reduce_max(double& val,int& idx,int size) const{
  /*! size is the maximum value of idx */
  //  VaId vi = {val, my_rank_*size +idx};
  //  VaId vo;
  double vIn = val;
  int idxIn = my_rank_*size +idx;
  int idxOut;
  BGNET_AllReduceLoc(&vIn,&idxIn,&val,&idxOut,1,BGNET_COLLECTIVE_DOUBLE,BGNET_COLLECTIVE_MAX,BGNET_COMM_WORLD);

  idx = idxOut%size;
  return idxOut/size;
}

int Communicator::reduce_min(double& val,int& idx,int size) const{
  /*! size is the maximum value of idx */
  //  VaId vi = {val, my_rank_*size +idx};
  //  VaId vo;
  double vIn = val;
  int idxIn = my_rank_*size +idx;
  int idxOut;
  BGNET_AllReduceLoc(&vIn,&idxIn,&val,&idxOut,1,BGNET_COLLECTIVE_DOUBLE,BGNET_COLLECTIVE_MIN,BGNET_COMM_WORLD);

  idx = idxOut%size;
  return idxOut/size;
}

int Communicator::pprintf(const char* format ...) const{
  va_list ap;
  int ret = 0;

  va_start(ap,format);
  if(my_rank_==0)
    ret = vfprintf( stdout, format, ap );
  va_end(ap);

  return ret;
}


#endif

