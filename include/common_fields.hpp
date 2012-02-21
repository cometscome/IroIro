/*!
 * @file common_fields.hpp
 *
 * @brief Code for general fields initializations
 */

#ifndef COMMON_FIELDS_H_
#define COMMON_FIELDS_H_

#include "Main/gaugeConf.hpp"
#include "include/geometry.hpp"
#include "include/field.h"
#include "include/format_A.h"
#include "include/format_G.h"
#include "include/format_F.h"
#include "lib/Tools/sunMat.hpp"
#include "include/macros.hpp"

struct ExtraDimTag {};
struct OneDimTag {};

/*!
 * @brief A Class to handle gauge fields
 * @author <a href="http://suchix.kek.jp/guido_cossu/">Guido Cossu</a> 
 * 
 * This class handles the Format 
 * and Field class in order to bind them 
 * into a single self contained object
 * that knows the details of storage
 *
 */
template < class DATA, class FORMAT, typename TAG = NullType> 
class GeneralField {
public:
  FORMAT format;
  DATA data;
  GeneralField();
  GeneralField(int);

  const FORMAT get_Format() const{ return format;}

  /*! 
   * Constructor\n 
   * to store given data
   */
  explicit GeneralField(const DATA&);

  GeneralField& operator=(const GeneralField& rhs);
  GeneralField& operator+=(const GeneralField& rhs);
  GeneralField& operator-=(const GeneralField& rhs);
  GeneralField& operator*=(const double& rhs);
  
  double norm();

};

template < class DATA, class FORMAT, typename TAG> 
GeneralField<DATA,FORMAT,TAG>:: 
GeneralField():format(FORMAT( CommonPrms::instance()->Nvol())),
	       data(format.size()){}

// Specialization for one dimension gauge field
template <> 
inline GeneralField< Field, Format::Format_G, OneDimTag>:: 
GeneralField():format(Format::Format_G( CommonPrms::instance()->Nvol(),1)),
	       data(format.size()){}

template < class DATA, class FORMAT, typename TAG> 
GeneralField<DATA,FORMAT,TAG>:: 
GeneralField(int LocalVol):format(FORMAT( LocalVol)),
			   data(format.size()){}

// Specialization for one dimension gauge field
template <> 
inline GeneralField< Field, Format::Format_G, OneDimTag>:: 
GeneralField(int LocalVol):format(Format::Format_G( LocalVol,1)),
			   data(format.size()){}


template < class DATA, class FORMAT, typename TAG> 
GeneralField<DATA,FORMAT,TAG>:: 
GeneralField(const DATA& FieldIn):format(FORMAT( CommonPrms::instance()->Nvol()))
{
  assert(FieldIn.size()== format.size());
  data = FieldIn;
}

// Specialization for one dimension gauge field
template <> 
inline GeneralField< Field, Format::Format_G, OneDimTag>::
GeneralField(const Field& FieldIn):format(Format::Format_G( CommonPrms::instance()->Nvol(),1))
{
  assert(FieldIn.size()== format.size());
  data = FieldIn;
}

template < class DATA, class FORMAT, typename TAG> 
inline GeneralField<DATA,FORMAT,TAG>& 
GeneralField<DATA,FORMAT,TAG>::operator=(const GeneralField& rhs)
{
  data = rhs.data;
  return *this;
}

template < class DATA, class FORMAT, typename TAG>
inline GeneralField<DATA,FORMAT,TAG>&
GeneralField<DATA,FORMAT,TAG>::operator+=(const GeneralField& rhs) 
{
  data += rhs.data;
  return *this;
}

template < class DATA, class FORMAT, typename TAG> 
inline GeneralField<DATA,FORMAT,TAG>&
GeneralField<DATA,FORMAT,TAG>::operator-=(const GeneralField& rhs) 
{
  data -= rhs.data;
  return *this;
}

template < class DATA, class FORMAT, typename TAG> 
inline GeneralField<DATA,FORMAT,TAG>&
GeneralField<DATA,FORMAT,TAG>::operator*=(const double& rhs) 
{
  data *= rhs;
  return *this;
}

template < class DATA, class FORMAT, typename TAG> 
double GeneralField<DATA,FORMAT,TAG>::norm() 
{
  return data.norm();
}

////////////////////////////////////////////////////////////////
// Specialization for extradimension

template < class DATA, class FORMAT> 
class GeneralField<DATA, FORMAT, ExtraDimTag> {
  FORMAT format;
  GeneralField(){}; //hide default constructor
public:
  DATA data;
  GeneralField(int Ls):format(FORMAT( CommonPrms::instance()->Nvol()*Ls)),
		       data(format.size()){}
  
  GeneralField(int Ls, int LocalVol):format(FORMAT( LocalVol*Ls)),
				     data(format.size()){}


  FORMAT get_Format(){ return format;}
  /*! 
   * Constructor\n 
   * to store given data
   */
  explicit GeneralField(DATA&);
  GeneralField& operator=(const GeneralField& rhs);
  GeneralField& operator+=(const GeneralField& rhs);
  GeneralField& operator-=(const GeneralField& rhs);
};



//////////////////////////////////////////////////////////////////////////////
// Notes
// GaugeField shoud contain also methods to create from geometry and initialize
typedef GeneralField< Field, Format::Format_A >              AdjGaugeField;
typedef GeneralField< Field, Format::Format_G >              GaugeField;
typedef GeneralField< Field, Format::Format_G, OneDimTag >   GaugeField1D;
typedef GeneralField< Field, Format::Format_F >              FermionField;
typedef GeneralField< Field, Format::Format_F, ExtraDimTag > FermionFieldExtraDim;
typedef GeneralField< std::vector<Field>, Format::Format_F > PropagatorField;


class GaugeGlobal: public GaugeField{
public:
  GaugeGlobal(Geometry geom){
    format = Format::Format_G( geom.parameters->Nvol());
    data = Field(format.size());
  }

 
  GaugeField& operator=(GaugeGlobal& Source){
    assert (format.size() == Source.format.size());
    data = Source.data;
    return *this;
  }

 /*!
   * Initializes the gauge field with an \n
   * external configuration in file <Filename>
   *
   * Configuration type in XML: TextFile
   * @param Filename String containing the filename
   */
  void initializeTxt(const std::string &Filename) {
    GaugeConf_txt gconf(format,Filename);
    gconf.init_conf(data);
  }

  /*!
   * Initializes the gauge field with an \n
   * external configuration in binary format
   * contained in file <Filename>
   *
   * Configuration type in XML: Binary
   * @param Filename String containing the filename
   */
  void initializeBin(const std::string &Filename) {
    GaugeConf_bin gconf(format,Filename);
    gconf.init_conf(data);
  }

  void initializeJLQCDlegacy(const std::string &Filename) {
    GaugeConf_JLQCDLegacy gconf(format,Filename);
    gconf.init_conf(data);
  }

 /*!
   * @brief Initializes the gauge field with \n
   * unit matrices
   *
   * Configuration type in XML: Unit
   */
  void initializeUnit(){
    GaugeConf_unit gconf(format);
    gconf.init_conf(data);
  }


  int initialize(XML::node node) {
    try {
      XML::descend(node, "Configuration");
      if (!XML::attribute_compare(node,"Type","TextFile")){
	std::string filename(node.child_value());
	initializeTxt(filename);
	return 0;
      }
      if (!XML::attribute_compare(node,"Type","Unit")){
	initializeUnit();
	return 0;
      }
      if (!XML::attribute_compare(node,"Type","Binary")){
	std::string filename(node.child_value());
	initializeBin(filename);
	return 0;
      }
      if (!XML::attribute_compare(node,"Type","JLQCDlegacy")){
	std::string filename(node.child_value());
	initializeJLQCDlegacy(filename);
	return 0;
      }
    } catch(...) {
      std::cout << "Error in initialization of gauge field "<< std::endl;
    }

    return 0;
  }

private:
  GaugeGlobal(const GaugeGlobal&);//hide copy constructor
};





namespace FieldUtils{
  const GaugeField TracelessAntihermite(const GaugeField&);

  // Field type-type transformations
  GaugeField1D DirSlice(const GaugeField& F, int dir);

  void SetMatrix(GaugeField& F, SUNmat mat, int site, int dir);
  void SetMatrix(GaugeField1D& F, SUNmat mat, int site);

  // Inline functions
  inline SUNmat matrix(const GaugeField& F, int site, int dir) {
    return SUNmat(F.data[F.format.cslice(0,site,dir)]);
  }
  inline SUNmat matrix(const GaugeField1D& F, int site){
    return SUNmat(F.data[F.format.cslice(0,site)]);
  }

  inline SUNmat matrix_dag(const GaugeField& F, int site, int dir){
    return SUNmat(F.data[F.format.cslice(0,site,dir)]).dag();
  }
  inline SUNmat matrix_dag(const GaugeField1D& F, int site){
    return SUNmat(F.data[F.format.cslice(0,site)]).dag();
  }

}



#endif //COMMON_FIELDS_H_
