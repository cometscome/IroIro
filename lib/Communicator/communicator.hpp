/*!
 * @file communicator.hpp
 *
 * @brief Declares the Communicator class for multiprocessing
 *
 * Time-stamp: <2013-04-25 14:39:25 neo>
 *
 */

#ifndef COMMUNICATOR_INCLUDED
#define COMMUNICATOR_INCLUDED

#include <vector>
#include <valarray>

class Communicator{
private:
  static int my_rank_;  /*!< rank of current process */
  static int Nproc_;    /*!< number of processes */

  enum{Ndim_max_ = 4}; // enum hack

  static int ipe_[Ndim_max_];
  static int nd_up_[Ndim_max_];
  static int nd_dn_[Ndim_max_];

  static void setup();
  Communicator(){
    setup(); // initializers and setup are called only once
  }
  Communicator(const Communicator&){}

  struct VaId{
    double value;
    int index;
  };

public:
  ~Communicator();
  static Communicator* instance();

  static int size()      {return Nproc_;}
  static int id()        {return my_rank_;}
  static int nodeid()    {return my_rank_;}
  static int ipe(int dir){return ipe_[dir];}
  static bool primaryNode();

  int nodeid(int x,int y,int z,int t) const;
  
  double reduce_sum(double)const;
  void sync()const;

  void broadcast(int size, int &data, int sender)const;
  void broadcast(int size, double &data, int sender)const;
  
  void allgather(double *bin,double *data,int size)const;
  void allgather(std::valarray<double>& bin,
		 const std::valarray<double>& data)const;

  void transfer_fw(double *bin,double *data,int size,int dir)const;

  void transfer_fw(std::valarray<double>& bin,
		   const std::valarray<double>& data,int dir)const;

  void transfer_fw(std::valarray<double>& bin,
		   const std::valarray<double>& data,
		   const std::vector<int>& index,int dir)const;

  void transfer_bk(double *bin,double *data,int size,int dir)const;

  void transfer_bk(std::valarray<double>& bin,
		   const std::valarray<double>& data,int dir)const;

  void transfer_bk(std::valarray<double>& bin,
		   const std::valarray<double>& data,
		   const std::vector<int>& index,int dir)const;

  void send_1to1(double *bin,double *data,int size,
		 int p_to, int p_from, int tag)const;
  void send_1to1(std::valarray<double>& bin, const std::valarray<double>& data,
		 int size, int p_to, int p_from, int tag)const;

  int reduce_max(double& val,int& idx,int size)const;
  int reduce_min(double& val,int& idx,int size)const;

};


#endif
