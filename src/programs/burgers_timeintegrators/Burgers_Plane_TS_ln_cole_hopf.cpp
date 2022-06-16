/*
 * Burgers_Plane_TS_ln_cole_hopf.cpp
 *
 *  Created on: 03 August 2017
 *      Author: Andreas Schmitt <aschmitt@fnb.tu-darmstadt.de>
 *
 */

#include "../burgers_timeintegrators/Burgers_Plane_TS_ln_cole_hopf.hpp"


void Burgers_Plane_TS_ln_cole_hopf::run_timestep(
		PlaneData_Spectral &io_u,	///< prognostic variables
		PlaneData_Spectral &io_v,	///< prognostic variables
		///PlaneData_Spectral &io_u_prev,	///< prognostic variables
		///PlaneData_Spectral &io_v_prev,	///< prognostic variables

		double i_fixed_dt,
		double i_simulation_timestamp
)
{
#if SWEET_PARAREAL
	if (simVars.parareal.enabled)
		SWEETError("Cole-Hopf solution is not usable in combination with Parareal");
#endif
	if (io_u.get_average() > 1e-12)
		SWEETError("Cole-Hopf solution does only work with functions which oszillate around 0");

	// setup dummy data
	PlaneData_Spectral tmp(io_u.planeDataConfig);
	PlaneData_Spectral phi(io_u.planeDataConfig);
//#if SWEET_USE_PLANE_SPECTRAL_SPACE
	tmp.spectral_set_zero();
	phi.spectral_set_zero();
//#endif
//	tmp.physical_set_all(0);
//	phi.physical_set_all(0);

	/*
	 * Calculating the analytic solution to the initial condition i_prog_u
	 * with Cole-Hopf transformation
	 */
	PlaneData_Spectral lhs = op.diff_c_x;

	tmp = io_u.spectral_div_element_wise(lhs);
//	phi = tmp;

	PlaneData_Physical phi_phys(phi.planeDataConfig);
	phi_phys = tmp.toPhys();

	phi_phys.physical_update_lambda_array_indices(
		[&](int i, int j, double &io_data)
		{
			io_data = exp(-io_data/(2*simVars.sim.viscosity));
		}
	);

	phi.loadPlaneDataPhysical(phi_phys);

	ts_l_direct.run_timestep(phi, io_v, /*io_u, io_v,*/ i_fixed_dt, i_simulation_timestamp);

	phi_phys = phi.toPhys();

	phi_phys.physical_update_lambda_array_indices(
		[&](int i, int j, double &io_data)
		{
			io_data = log(io_data);
		}
	);

	phi.loadPlaneDataPhysical(phi_phys);

	io_u = -2*simVars.sim.viscosity*op.diff_c_x(phi);

}



/*
 * Setup
 */
void Burgers_Plane_TS_ln_cole_hopf::setup()
{
	ts_l_direct.setup();
}


Burgers_Plane_TS_ln_cole_hopf::Burgers_Plane_TS_ln_cole_hopf(
		SimulationVariables &i_simVars,
		PlaneOperators &i_op
)	:
		simVars(i_simVars),
		op(i_op),
		ts_l_direct(i_simVars, i_op)
{
	setup();
}



Burgers_Plane_TS_ln_cole_hopf::~Burgers_Plane_TS_ln_cole_hopf()
{
}

