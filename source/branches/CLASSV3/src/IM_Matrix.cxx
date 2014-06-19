//
//  IM_Matrix.cxx
//  CLASSSource
//
//  Created by BaM on 04/05/2014.
//  Copyright (c) 2014 BaM. All rights reserved.
//

#include "IM_Matrix.hxx"

#include "IsotopicVector.hxx"
#include "CLASSHeaders.hxx"
#include "LogFile.hxx"
#include "StringLine.hxx"

#include <TGraph.h>
#include <TString.h>


#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>



using namespace std;


//________________________________________________________________________
IM_Matrix::IM_Matrix():DynamicalSystem()
{
	fShorstestHalflife = 3600.*24*160.; //cut by default all nuclei with a shorter liftime than the Cm242 -> remain 33 actinides
}


IM_Matrix::IM_Matrix(LogFile* log):IrradiationModel(log), DynamicalSystem()
{
	fShorstestHalflife = 3600.*24*160.; //cut by default all nuclei with a shorter liftime than the Cm242 -> remain 33 actinides
}




//________________________________________________________________________
/*			Evolution Calculation			*/
//________________________________________________________________________
EvolutionData IM_Matrix::GenerateEvolutionData(IsotopicVector isotopicvector, EvolutionData XSSet, double Power, double cycletime)
{

	if(fFastDecay.size() == 0)
	{
		BuildDecayMatrix();
		fNVar = findex_inver.size();
	}

	string ReactorType;


	vector< TMatrixT<double> > NMatrix ;//  TMatrixT<double>(decayindex.size(),1))
	{	// Filling the t=0 State;
		map<ZAI, double > isotopicquantity = isotopicvector.GetIsotopicQuantity();
		TMatrixT<double>  N_0Matrix =  TMatrixT<double>( findex.size(),1) ;
		for(int i = 0; i < (int)findex.size(); i++)
			N_0Matrix[i] = 0;

		map<ZAI, double >::iterator it ;
		for(int i = 0; i < (int)findex.size(); i++)
			N_0Matrix[i] = 0;

		for(it = isotopicquantity.begin(); it != isotopicquantity.end(); it++)
		{
			/// Need TO change with FP managment
			map<ZAI, int >::iterator it2;

			if( (*it).first.Z() < fZAIThreshold )
				it2 = findex_inver.find( ZAI(-2,-2,-2) );
			else it2 = findex_inver.find( (*it).first );

			if(it2 == findex_inver.end() )		//If not in index should be TMP, can't be fast decay for new Fuel !!!
				it2 = findex_inver.find( ZAI(-3,-3,-3) );
			N_0Matrix[ (*it2).second ][0] = (*it).second ;


		}

		isotopicquantity.clear();

		NMatrix.push_back(N_0Matrix);
		N_0Matrix.Clear();

	}


	//-------------------------//
	//--- Perform Evolution ---//
	//-------------------------//
	ReactorType = XSSet.GetReactorType();

	double Na = 6.02214129e23;	//N Avogadro
	double M_ref = XSSet.GetHeavyMetalMass();
	double M = 0;
	double Power_ref =  XSSet.GetPower();

	// Get the mass of the fuel to irradiate in order to perform the evolution at fixed burnup (the burnup step of the calculation will match the burnup step of the XSSet
	{
		map<ZAI, double >::iterator it ;


		IsotopicVector IVtmp = isotopicvector.GetActinidesComposition();
		map<ZAI, double >isotopicquantity = IVtmp.GetIsotopicQuantity();

		for( it = isotopicquantity.begin(); it != isotopicquantity.end(); it++ )
			M += isotopicvector.GetActinidesComposition().GetZAIIsotopicQuantity( (*it).first )*cZAIMass.fZAIMass.find( (*it).first )->second/Na*1e-6;
		isotopicquantity.clear();

	}

	int NStep = XSSet.GetFissionXS().begin()->second->GetN();
	double* DBTimeStep = XSSet.GetFissionXS().begin()->second->GetX();

	int InsideStep = 10;

	double timevector[NStep];
	timevector[0] = 0;

	double  Flux[NStep];

	TMatrixT<double> FissionEnergy = TMatrixT<double>(findex.size(),1);
	for(int i = 0; i < (int)findex.size(); i++)
		FissionEnergy[i] = 0;

	{
		map< ZAI, int >::iterator it;
		for(it = findex_inver.begin(); it != findex_inver.end(); it++)
		{
			map< ZAI, double >::iterator it2 = fFissionEnergy.find(it->first);
			if(it2 == fFissionEnergy.end())
			{
				if(it->first.Z() > fZAIThreshold)
					FissionEnergy[it->second][0] = 1.9679e6*it->first.A()-2.601e8; // //simple linear fit to known values ;extrapolation to unknown isotopes
				else FissionEnergy[it->second][0] = 0;
			}
			else
				FissionEnergy[it->second][0] = it2->second;

		}
	}

	vector< TMatrixT<double> > FissionXSMatrix;	// Store The Fisison XS Matrix
	vector< TMatrixT<double> > CaptureXSMatrix;	// Store The Capture XS Matrix
	vector< TMatrixT<double> > n2nXSMatrix;		// Store The n2N XS Matrix

	for(int i = 0; i < NStep-1; i++)
	{
		double TStepMax = ( (DBTimeStep[i+1]-DBTimeStep[i] ) ) * Power_ref/M_ref / Power*M ;	// Get the next Time step


		TMatrixT<double> BatemanMatrix = TMatrixT<double>(findex.size(),findex.size());
		TMatrixT<double> BatemanReactionMatrix = TMatrixT<double>(findex.size(),findex.size());

		TMatrixT<double> NEvolutionMatrix = TMatrixT<double>(findex.size(),1);
		NEvolutionMatrix = NMatrix.back();



		FissionXSMatrix.push_back(GetFissionXsMatrix(XSSet, DBTimeStep[i]));	//Feel the fission reaction Matrix
		CaptureXSMatrix.push_back(GetCaptureXsMatrix(XSSet, DBTimeStep[i]));	//Feel the capture reaction Matrix
		n2nXSMatrix.push_back(Getn2nXsMatrix(XSSet, DBTimeStep[i]));		//Feel the (n,2n)  reaction Matrix

		// ----------------   Evolution

		BatemanReactionMatrix = FissionXSMatrix[i];
		BatemanReactionMatrix += CaptureXSMatrix[i];
		BatemanReactionMatrix += n2nXSMatrix[i];

		for(int k=0; k < InsideStep; k++)
		{
			double ESigmaN = 0;
			for (int j = 0; j < (int)findex.size() ; j++)
				ESigmaN -= FissionXSMatrix[i][j][j]*NEvolutionMatrix[j][0]*1.6e-19*FissionEnergy[j][0];
			// Update Flux
			double Flux_k = Power/ESigmaN;

			if(k==0)
				Flux[i]=Flux_k;

			BatemanMatrix = BatemanReactionMatrix;
			BatemanMatrix *= Flux_k;
			BatemanMatrix += fDecayMatrix ;
			BatemanMatrix *= TStepMax/InsideStep ;


			TMatrixT<double> IdMatrix = TMatrixT<double>(findex.size(),findex.size());
			for(int j = 0; j < (int)findex.size(); j++)
				for(int k = 0; k < (int)findex.size(); k++)
				{
					if(k == j)	IdMatrix[j][k] = 1;
					else 		IdMatrix[j][k] = 0;
				}


			TMatrixT<double> BatemanMatrixDL = TMatrixT<double>(findex.size(),findex.size());   // Order 0 Term from the DL : Id
			TMatrixT<double> BatemanMatrixDLTermN = TMatrixT<double>(findex.size(),findex.size());  // Addind it;

			{
				BatemanMatrix *= TStepMax ;
				BatemanMatrixDLTermN = IdMatrix;
				BatemanMatrixDL = BatemanMatrixDLTermN;
				int j = 1;
				double NormN;

				do
				{
					TMatrixT<double> BatemanMatrixDLTermtmp = TMatrixT<double>(findex.size(),findex.size());  // Adding it;
					BatemanMatrixDLTermtmp = BatemanMatrixDLTermN;

					BatemanMatrixDLTermN.Mult(BatemanMatrixDLTermtmp, BatemanMatrix );

					BatemanMatrixDLTermN *= 1./j;
					BatemanMatrixDL += BatemanMatrixDLTermN;

					NormN = 0;
					for(int m = 0; m < (int)findex.size(); m++)
						for(int n = 0; n < (int)findex.size(); n++)
							NormN += BatemanMatrixDLTermN[m][n]*BatemanMatrixDLTermN[m][n];
					j++;

				} while ( NormN != 0 );
			}
			NEvolutionMatrix = BatemanMatrixDL * NEvolutionMatrix ;
		}
		NMatrix.push_back(NEvolutionMatrix);


		timevector[i+1] = timevector[i] + TStepMax;

		BatemanMatrix.Clear();
		BatemanReactionMatrix.Clear();
		NEvolutionMatrix.Clear();


	}
	FissionXSMatrix.push_back(GetFissionXsMatrix(XSSet, DBTimeStep[NStep-1])); //Feel the reaction Matrix
	CaptureXSMatrix.push_back(GetCaptureXsMatrix(XSSet, DBTimeStep[NStep-1])); //Feel the reaction Matrix
	n2nXSMatrix.push_back(Getn2nXsMatrix(XSSet, DBTimeStep[NStep-1])); //Feel the reaction Matrix


	EvolutionData GeneratedDB = EvolutionData(GetLog());

	double ESigmaN = 0;
	for (int j = 0; j < (int)findex.size() ; j++)
		ESigmaN -= FissionXSMatrix.back()[j][j]*NMatrix.back()[j][0]*1.6e-19*FissionEnergy[j][0];

	Flux[NStep-1] = Power/ESigmaN;

	GeneratedDB.SetFlux( new TGraph(NStep, timevector, Flux)  );

	for(int i = 0; i < (int)findex.size(); i++)
	{
		double ZAIQuantity[NMatrix.size()];
		double FissionXS[NStep];
		double CaptureXS[NStep];
		double n2nXS[NStep];
		for(int j = 0; j < (int)NMatrix.size(); j++)
			ZAIQuantity[j] = (NMatrix[j])[i][0];

		for(int j = 0; j < NStep; j++)
		{
			FissionXS[j]	= FissionXSMatrix[j][i][i];
			CaptureXS[j]	= CaptureXSMatrix[j][i][i];
			n2nXS[j]	= n2nXSMatrix[j][i][i];
		}

		GeneratedDB.NucleiInsert(pair<ZAI, TGraph*> (findex.find(i)->second, new TGraph(NMatrix.size(), timevector, ZAIQuantity)));
		GeneratedDB.FissionXSInsert(pair<ZAI, TGraph*> (findex.find(i)->second, new TGraph(NStep, timevector, FissionXS)));
		GeneratedDB.CaptureXSInsert(pair<ZAI, TGraph*> (findex.find(i)->second, new TGraph(NStep, timevector, CaptureXS)));
		GeneratedDB.n2nXSInsert(pair<ZAI, TGraph*> (findex.find(i)->second, new TGraph(NStep, timevector, n2nXS)));
	}

	GeneratedDB.SetPower(Power );
	//	GeneratedDB.SetFuelType(fFuelType );
	GeneratedDB.SetReactorType(ReactorType );
	GeneratedDB.SetCycleTime(cycletime);

	for (int i = 0; i < (int) FissionXSMatrix.size(); i++)
	{
		FissionXSMatrix[i].Clear();
		CaptureXSMatrix[i].Clear();
		n2nXSMatrix[i].Clear();
	}
	FissionXSMatrix.clear();
	CaptureXSMatrix.clear();
	n2nXSMatrix.clear();
	
	return GeneratedDB;
	
}
















