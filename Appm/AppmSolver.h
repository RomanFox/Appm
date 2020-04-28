#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <iomanip>

#include "PrimalMesh.h"
#include "DualMesh.h"
#include "Numerics.h"
#include "FluidSolver.h"
#include "SingleFluidSolver.h"
#include "TwoFluidSolver.h"
#include "MultiFluidSolver.h"

#include "MaxwellSolver.h"
#include "MaxwellSolverCrankNicholson.h"
#include "MaxwellSolverImplicitEuler.h"

#include "Physics.h"

#include <Eigen/SparseLU>


#define _RT_ONECELL 
#undef _RT_ONECELL


class AppmSolver
{

public:
	AppmSolver();
	AppmSolver(const PrimalMesh::PrimalMeshParams & primalMeshParams);
	~AppmSolver();

	void run();

protected:
	PrimalMesh primalMesh;
	DualMesh dualMesh;

	Eigen::Matrix3Xd B_vertex;

	bool isMaxwellCurrentSource = false;

private:

	struct ParticleParameters {
		std::string name = "neutral";
		double mass = 1.0;
		int electricCharge = 0;
	};
	std::vector<ParticleParameters> particleParams;

	enum class MassFluxScheme {
		EXPLICIT, IMPLICIT_EXPLICIT

	};
	friend std::ostream & operator<<(std::ostream & os, const MassFluxScheme & obj);


	/** Get number of fluids. */
	const int getNFluids() const;

	bool isStateWrittenToOutput = false;



	bool isMaxwellEnabled = false;
	bool isFluidEnabled = true;
	double timestepSize = 1.0;
	int maxIterations = 0;
	double maxTime = 0;
	double lambdaSquare = 1.0;
	MassFluxScheme massFluxScheme = MassFluxScheme::EXPLICIT;
	int initType = 1;
	bool isElectricLorentzForceActive = false;
	bool isMagneticLorentzForceActive = false;

	enum class MaxwellSolverBCType {
		VOLTAGE_BC, CURRENT_BC
	};
	friend std::ostream & operator<<(std::ostream & os, const MaxwellSolverBCType & obj);

	MaxwellSolverBCType maxwellSolverBCType = MaxwellSolverBCType::CURRENT_BC;

	bool isWriteEfield = false;
	bool isWriteBfield = false;
	bool isWriteHfield = false;
	
	// State vector for solving Maxwell's equations
	Eigen::VectorXd maxwellState;
	Eigen::VectorXd maxwellStatePrevious;
	Eigen::SparseMatrix<double> Q;
	Eigen::SparseMatrix<double> Meps;
	Eigen::SparseMatrix<double> Mnu;
	Eigen::SparseMatrix<double> C;

	// M1 = lambda^2 * Q' * Meps * Q in the reformulated Ampere equation. 
	Eigen::SparseMatrix<double> M1;

	// M2 = Cdual * Mnu * C in the reformulated Ampere equation, restricted to inner edges. 
	Eigen::SparseMatrix<double> M2;

	Eigen::VectorXd E_h;
	Eigen::VectorXd B_h;
	Eigen::VectorXd J_h;
	Eigen::VectorXd J_h_previous;

	Eigen::Matrix3Xd E_cc; // Electric field at cell center



	Eigen::MatrixXd fluidStates;
	Eigen::MatrixXd fluidStates_new;
	Eigen::MatrixXd fluidSources;
	Eigen::MatrixXd fluidFluxes;
	Eigen::MatrixXd faceFluxes;

	Eigen::MatrixXd faceFluxesImExRusanov;

	const int faceIdxRef = -1;

	// Isentropic expansion coefficient, aka ratio of heat capacities
	//const double gamma = 1.4; 

	void init_maxwellStates();
	Eigen::SparseMatrix<double> getBoundaryGradientInnerInclusionOperator();
	Eigen::SparseMatrix<double> getElectricPermittivityOperator();
	Eigen::SparseMatrix<double> getMagneticPermeabilityOperator();

	void init_multiFluid(const std::string & filename);

	void init_SodShockTube(const double zRef);
	void init_Uniformly(const double n, const double p, const double u);
	void init_Explosion(const Eigen::Vector3d refPos, const double radius);

	void set_Efield_uniform(const Eigen::Vector3d direction);

	Eigen::SparseMatrix<double> M_perot;
	void initPerotInterpolationMatrix();
	const Eigen::Matrix3Xd getEfieldAtCellCenter();

	// Matrix for Ohms law J_h = M_sigma * E_h
	Eigen::SparseMatrix<double> M_sigma;
	void initMsigma();

	const double terminalVoltageBC_sideA(const double time, const double t0, const double tscale) const;
	const double terminalVoltageBC_sideB(const double time) const;
	const double currentDensityBC(const double time) const;

	const int getFluidStateLength() const;
	const double getNextFluidTimestepSize() const;

	//const Eigen::VectorXd getFluidState(const int cellIdx, const int fluidIdx) const;
	const Eigen::Vector3d getFluidState(const int cellIdx, const int fluidIdx, const Eigen::Vector3d & faceNormal) const;
	
	const int getOrientation(const Cell * cell, const Face * face) const;
	
	//const Eigen::Vector3d getFluidFluxFromState(const Eigen::Vector3d & q) const;
	const Eigen::Vector3d getRusanovFluxExplicit(const int faceIdx, const int fluidIdx) const;
	const Eigen::Vector3d getRusanovFluxImEx(const int faceIdx, const int fluidIdx, const double dt);

	const double getImplicitExtraTermMomentumFlux(const int cellIdx, const Eigen::Vector3d & faceNormal, const int fluidIdx) const;

	const std::pair<int,int> getAdjacientCellStates(const int faceIdx, const int fluidIdx, Eigen::Vector3d & qL, Eigen::Vector3d & qR) const;

	const double getMomentumUpdate(const int k, const Eigen::Vector3d & nvec, const int fluidIdx) const;
	

	void interpolateMagneticFluxToPrimalVertices();

	// void test_raviartThomas();
	const Eigen::Matrix3Xd getPrismReferenceCoords(const int nSamples);

	std::vector<double> timeStamps;

	void init_meshes(const PrimalMesh::PrimalMeshParams & primalParams);

	void writeXdmf();
	void writeXdmfDualVolume();
	
	void writeOutput(const int iteration, const double time);

	void writeFluidStates(H5Writer & writer);
	void writeMaxwellStates(H5Writer & writer);

	XdmfGrid getOutputPrimalEdgeGrid(const int iteration, const double time, const std::string & dataFilename);
	XdmfGrid getOutputPrimalSurfaceGrid(const int iteration, const double time, const std::string & dataFilename);
	
	XdmfGrid getOutputDualEdgeGrid(const int iteration, const double time, const std::string & dataFilename);
	XdmfGrid getOutputDualSurfaceGrid(const int iteration, const double time, const std::string & dataFilename);
	XdmfGrid getOutputDualVolumeGrid(const int iteration, const double time, const std::string & dataFilename);

	void init_RaviartThomasInterpolation();

	std::vector<Eigen::Matrix3d> rt_piolaMatrix;
	std::vector<Eigen::Vector3d> rt_piolaVector;

	void readParameters(const std::string & filename);

	const std::string xdmf_GridPrimalEdges(const int iteration) const;
	const std::string xdmf_GridPrimalFaces(const int iteration) const;
	const std::string xdmf_GridDualEdges(const int iteration) const;
	const std::string xdmf_GridDualFaces(const int iteration) const;
	const std::string xdmf_GridDualCells(const int iteration) const;

	const std::string fluidXdmfOutput(const std::string & filename) const;
};

