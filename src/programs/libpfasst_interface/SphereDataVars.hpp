#ifndef _SPHERE_DATA_VARS_HPP_
#define _SPHERE_DATA_VARS_HPP_

#include <sweet/sphere/SphereData_Config.hpp>
#include <sweet/sphere/SphereData_Spectral.hpp>

// Class containing the prognostic SphereDataSpectral variables phi_pert, vrt, and div

class SphereDataVars {

public:

  // Constructor
  SphereDataVars(
                SphereData_Config *sphereDataConfig,
		int i_level
		)

    : prog_phi_pert(sphereDataConfig),
      prog_vrt(sphereDataConfig),
      prog_div(sphereDataConfig),
      flat_data_array(nullptr),
      flat_data_array_size(0),
      level(i_level)
  {}
  
  // Destructor
  ~SphereDataVars()
  {
    if (flat_data_array != nullptr) 
      {
	// release the memory
	MemBlockAlloc::free(
			    flat_data_array,
			    flat_data_array_size*sizeof(double)
			    );
      }
  }

  void allocate_flat_data_array(int i_n_elems) 
  {
    if (flat_data_array == nullptr) 
      {
	// allocate the memory
	flat_data_array_size = i_n_elems;
	flat_data_array      = MemBlockAlloc::alloc<double>(
							    i_n_elems*sizeof(double)
							    );
      }						   
  }
  
  // getters for the SphereDataSpectral variables
  const SphereData_Spectral& get_phi_pert() const  {return prog_phi_pert;}
  SphereData_Spectral&       get_phi_pert()        {return prog_phi_pert;}
  const SphereData_Spectral& get_vrt() const {return prog_vrt;}
  SphereData_Spectral&       get_vrt()       {return prog_vrt;}
  const SphereData_Spectral& get_div() const  {return prog_div;}
  SphereData_Spectral&       get_div()        {return prog_div;}

  // getters for the flat data array
  double*&         get_flat_data_array()            {return flat_data_array;}
  const int&       get_flat_data_array_size() const {return flat_data_array_size;}
  int&             get_flat_data_array_size()       {return flat_data_array_size;}

  // getters for the level 
  const int&       get_level() const {return level;};

protected:
  
  SphereData_Spectral prog_phi_pert;
  SphereData_Spectral prog_vrt;
  SphereData_Spectral prog_div;

  // flat data array vector (currently used to pack and unpack)
  double *flat_data_array;
  int     flat_data_array_size;
  
  // pfasst level
  const int level;

  // default constructor, copy constructor, and operator= are disabled
  SphereDataVars();
  SphereDataVars(const SphereDataVars&);
  SphereDataVars& operator=(const SphereDataVars&);

};

#endif
