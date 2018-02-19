#ifndef FLUID_SOLVER_3D_H_
#define FLUID_SOLVER_3D_H_

#include <string>
#include <vector>
#include <fstream>
#include <climits>
#include <glm/glm.hpp>
#include <iostream>
#include <exception>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <limits>
//Timing
#include <chrono>
#include <ratio>

#include "SimUtil.h"
#include "MarchingCubes.h"
#include "classicSolver.h"
#include "timing.h"


class FluidSolver3D {

private:
	//----------------------------------------------------------------------
	// Grid Attributes
	//----------------------------------------------------------------------

	// nx
	int m_gridWidth;
	// ny
	int m_gridHeight;
	// nz
	int m_gridDepth;
	// the max of depth, height and width
	int m_maxGridSize;
	// distance between each grid cell
	float m_dx;
	// grid of cell labels, size (nx, ny)
	SimUtil::Mat3Di m_label;

	// pressure and velocity are held in a MAC grid so that
	// p(i, j, k) = p_i_j_k
	// u(i, j, k) = u_i-1/2_j_k
	// v(i, j, k) = v_i_j-1/2_k
	// w(i, j, k) = w_i_j_k-1/2

	// grid of pressures, size (nx, ny, nz)
	SimUtil::Mat3Df m_p;
	// grid of vel x component, size (nx+1, ny, nz)
	SimUtil::Mat3Df m_u;
	// grid of vel y component, size (nx, ny+1, nz)
	SimUtil::Mat3Df m_v;
	// grid of vel z component, size (nx, ny, nz+1)
	SimUtil::Mat3Df m_w;
	// saved grid of vel x component for FLIP update, size (nx+1, ny, nz)
	SimUtil::Mat3Df m_uSaved;
	// saved grid of vel y component for FLIP update, size (nx, ny+1, nz)
	SimUtil::Mat3Df m_vSaved;
	// saved grid of vel z component for FLIP update, size (nx, ny, nz + 1)
	SimUtil::Mat3Df m_wSaved;

	//----------------------------------------------------------------------
	// Simulation Attributes
	//----------------------------------------------------------------------

	const int VEL_UNKNOWN = INT_MIN;
	// number of particles to seed in each cell at start of sim
	const int PARTICLES_PER_CELL = 8;
	// the amount of weight to give to PIC in PIC/FLIP update
	const float PIC_WEIGHT = 0.02f;
	// the maximum number of grid cells a particle should move when advected
	const int ADVECT_MAX = 1;
	// acceleration due to gravity
	const float GRAVITY = 9.81f;
	// density of the fluid (kg/m^3)
	const float FLUID_DENSITY = 1000.0f;
	// surface threshold for marching cubes
	//const float SURFACE_THRESHOLD = 5.0f;
	const float SURFACE_THRESHOLD = 0.0f;

	// simulation time step
	float m_dt;
	// current orientation
	SimUtil::Vec3 m_orientation{ 0.0f, -1.0f, 0.0f };

	//----------------------------------------------------------------------
	// Particle-related Members
	//----------------------------------------------------------------------

	// list of all particles in the simulation
	std::vector<SimUtil::Particle3D> *m_particles;

	//----------------------------------------------------------------------
	// For Output Purposes
	//----------------------------------------------------------------------

	// list of all cases for marching cubes
	std::vector<std::vector<glm::vec3>> m_cubeCases;
	std::vector<std::vector<int>> m_cubeIndices;

	//----------------------------------------------------------------------
	// For Timing Purposes
	//----------------------------------------------------------------------
	
	// timing class object
	timing *m_timer;
	// definies if timing class is initialized
	const bool ENABLE_TIMING = true;

	//----------------------------------------------------------------------
	// Functions
	//----------------------------------------------------------------------

	// solver steps
	void seedParticles(int, std::vector<SimUtil::Particle3D>*);
	void labelGrid();
	void particlesToGrid();
	void extrapolateGridFluidData(SimUtil::Mat3Df &grid, int x, int y, int z, int extrapolationDepth);
	void saveVelocityGrids();
	void applyBodyForces();
	void applyPressure();
	void gridToParticles(float);
	void advectParticles(int);
	void cleanupParticles(float);

	// helper functions
	double trilinearHatKernel(SimUtil::Vec3);
	double hatFunction(double);
	std::vector<int> checkNeighbors(SimUtil::Mat3Di&, int[3], int[3], int[][3], int, int);
	bool hasNeighbors(SimUtil::Mat3Di&, int[3], int[3], int[][3], int, int);
	SimUtil::Vec3 interpVel(SimUtil::Mat3Df&, SimUtil::Mat3Df&, SimUtil::Mat3Df&, SimUtil::Vec3);
	void RK3(SimUtil::Particle3D*, SimUtil::Vec3, float, SimUtil::Mat3Df&, SimUtil::Mat3Df&, SimUtil::Mat3Df&);
	bool projectParticle(SimUtil::Particle3D *, float);
	std::vector<std::string> split(std::string str, std::string token);
	

	// debugging functions
	void gridValues(SimUtil::Mat3Df &grid, std::string name, int x, int y, int z);
	void strangeParticles();

public:
	/*
	Creates a new 2D fluid solver.
	Args:
	width - width of the grid to use
	height - height of the grid to use
	depth - depth of the grid to use
	dx - the grid cell width
	dt - the timestep to use
	*/
	FluidSolver3D(int, int, int, float, float);
	~FluidSolver3D();

	/*
	Initializes the solver by reading in and constructing initial
	grid based on the given initial geometry file. The solver will save particle
	data at each time step to the given output file.
	Args:
	initialGemoetryFile - name of the .txt file containing initial geometry
	*/
	void init(std::string);

	/*
	Update the force orientation with a new given orientation
	Args:
	orientation - vector with the orientation of the force {0,1,0} for standard
	*/
	void updateOrientation(glm::vec3 orientation) { m_orientation.x = orientation.x; m_orientation.y = orientation.y; m_orientation.z = orientation.z;};

	/*
	Steps the simulation forward dt.
	*/
	void step();
	/*
	Times the different algorithms in step()
	*/
	void stepTiming();
	/*
	Saves the lines of the isocontur representing the surface between water and
	air. Outputs in csv format where each line is represented by two points.
	each coordinate and each point is seperated by a space. Each line is one
	timestep.
	Args:
	LinesOut - pointer to file stream to use for output
	*/
	void saveParticleData(std::ofstream*);

	/*
	Returns the particles location as vectors
	*/
	std::vector<glm::vec3> particleData();

	/*
	Returns the triangle data of the isocontour where the pressure is zero in the 
	current pressure grid as a struct of 3 vectors.
	first containing vertices as vec3
	second containing normals as vec3
	third containing vertex indices for index buffering
	*/
	SimUtil::Mesh3D meshData();
	/*
	Saves the average timing data for each sub-algorithm of the step algorithm
	*/
	void saveTimingData(std::ofstream*);

	SimUtil::Mat3Di* getGeometry() {
		return &m_label;
	};
};

#endif //FLUID_SOLVER_3D_H_