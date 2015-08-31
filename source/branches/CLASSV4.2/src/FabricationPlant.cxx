#include "FabricationPlant.hxx"

#include "Storage.hxx"
#include "Reactor.hxx"
#include "EvolutionData.hxx"
#include "DecayDataBank.hxx"
#include "PhysicsModels.hxx"
#include "IsotopicVector.hxx"
#include "Scenario.hxx"
#include "CLASSLogger.hxx"
#include "CLASSConstante.hxx"

#include "TMatrixT.h"

#include <sstream>
#include <string>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <stdarg.h> 

ClassImp(FabricationPlant)

	//________________________________________________________________________
	//________________________________________________________________________
	//
	//		FabricationPlant
	//________________________________________________________________________
	//________________________________________________________________________


FabricationPlant::FabricationPlant():CLASSFacility(16)
{
	SetName("F_FabricationPLant.");
	
	fReUsable = 0;
	fIsReusable = false;
}


FabricationPlant::FabricationPlant(CLASSLogger* log, double fabricationtime):CLASSFacility(log, fabricationtime, 16)
{
DBGL
	SetName("F_FabricationPLant.");

	fFiFo = false;
	fSubstitutionFuel = false;

	fReUsable = 0;
	fIsReusable = false;
	
	INFO	<< " A FabricationPlant has been define :" << endl;
	INFO	<< "\t Chronological Stock Priority has been set! "<< endl;
	INFO	<< "\t Fabrication time set to \t " << (double)(GetCycleTime()/cYear) << " year" << endl << endl;
DBGL
}


	//________________________________________________________________________
FabricationPlant::~FabricationPlant()
{
	
	
}

	//________________________________________________________________________
void FabricationPlant::SetSeparartionEfficiencyIV(ZAI zai, double factor)
{

	pair<map<ZAI, double>::iterator, bool> IResult;
	if(factor > 1) factor = 1;
	
	if(factor > 0)
	{
		IResult =  fSeparationLostFraction.GetIsotopicQuantity().insert( pair<ZAI ,double>(zai, 1 - factor));
		if(!IResult.second)
			IResult.first->second = 1 - factor;
	}
	
}

	//________________________________________________________________________
	//_______________________________ Evolution ______________________________
	//________________________________________________________________________
void FabricationPlant::Evolution(cSecond t)
{
	
		// Check if the FabricationPlant has been created ...
	if(t == fInternalTime && t != 0) return;
		// Make the evolution for the FabricationPlant ...
	FabricationPlantEvolution(t);
		//Update Inside IsotopicVector
	UpdateInsideIV();
		// ... And Finaly update the AbsoluteInternalTime
	fInternalTime = t;
	
}

	//________________________________________________________________________
void FabricationPlant::FabricationPlantEvolution(cSecond t)
{
DBGL

	map<int ,cSecond >::iterator it;
	for( it = fReactorNextStep.begin(); it!= fReactorNextStep.end(); it++ )
	{
		double R_CreactionTime = GetParc()->GetReactor()[ (*it).first ]->GetCreationTime();
		double R_LifeTime 	   = GetParc()->GetReactor()[ (*it).first ]->GetLifeTime();

		int ReactorId = (*it).first;
		pair<CLASSFuel, double> R_Fuel = GetParc()->GetReactor()[ReactorId]->GetFuelPlan()->GetFuelAt( t + GetCycleTime() );
		double R_BU 		= R_Fuel.second;
		double R_Power 	= GetParc()->GetReactor()[ReactorId]->GetPower();
		double R_HMMass 	= GetParc()->GetReactor()[ReactorId]->GetHeavyMetalMass();
		cSecond R_CycleTime 	= (cSecond) (R_BU*1e9 / (R_Power) * R_HMMass * 3600 * 24);
		if( R_CycleTime < GetCycleTime())
		{
			ERROR << "Reactor Cycle Time is shorter than Fabrication Time of the fuel, we cannot deal it upto now!!!"<< endl;
			exit(1);
		}

		if( t + GetCycleTime() >= R_CreactionTime
		   && t + GetCycleTime() < R_CreactionTime + R_LifeTime)
		{
			if( (*it).second == t )
			{
#pragma omp critical(FuelBuild)
				{
					if( R_Fuel.first.GetPhysicsModels() )
					{
						BuildFuelForReactor( (*it).first, t ); 
					}
					(*it).second += R_CycleTime;
				}

			}
			else if ( (*it).second - R_CycleTime + GetCycleTime() >= t && (*it).second - R_CycleTime  < t )
			{
				map<int ,IsotopicVector >::iterator it2 = fReactorFuturIV.find( (*it).first );
				if (it2 != fReactorFuturIV.end())
					(*it2).second = GetDecay((*it2).second, t - fInternalTime );		
			}
		}
	}
	
	
DBGL
}

void FabricationPlant::UpdateInsideIV()
{
	DBGL
	fInsideIV = IsotopicVector();

	map< int,IsotopicVector >::iterator it;
	for( it = fReactorFuturIV.begin(); it != fReactorFuturIV.end(); it++ )
		fInsideIV += (*it).second;

	DBGL
}


	//________________________________________________________________________
void FabricationPlant::BuildFuelForReactor(int ReactorId, cSecond t)
{
	DBGL

	double R_HM_Mass	= GetParc()->GetReactor()[ ReactorId ]->GetHeavyMetalMass();
	double R_CycleTime	= GetParc()->GetReactor()[ ReactorId ]->GetCycleTime();
	double R_Power	= GetParc()->GetReactor()[ ReactorId ]->GetPower();

	pair<CLASSFuel, double > FuelBU  	= GetParc()->GetReactor()[ReactorId]->GetFuelPlan()->GetFuelAt(t+GetCycleTime()) ;
	PhysicsModels FuelType 		= *FuelBU.first.GetPhysicsModels();
	double R_BU	      			= FuelBU.second;

	fStreamList = FuelType.GetEquivalenceModel()->GetAllStreamList();

	BuildArray(ReactorId);

	map < string , vector <double> > LambdaArray =  FuelType.GetEquivalenceModel()->BuildFuel(R_BU, R_HM_Mass, fStreamArray);

	map < string , vector <IsotopicVector> >::iterator it_s_vIV;
	map < string , vector <double> >::iterator it_s_vD;
	map < string , bool >::iterator it_s_B;

	bool FuelCanBeBuilt 	= true; 
	double  LambdaSum 	= 0;
	
	for( it_s_vD = LambdaArray.begin();  it_s_vD != LambdaArray.end(); it_s_vD++)
	{	
		for(int i = 0; i < (int)LambdaArray[(*it_s_vD).first].size();i++) {fErrorOnLambda[(*it_s_vD).first] = false;}
	}

	for( it_s_vD = LambdaArray.begin();  it_s_vD != LambdaArray.end(); it_s_vD++)
	{	
		for(int i = 0; i < (int)LambdaArray[(*it_s_vD).first].size();i++)
		{
			if(LambdaArray[(*it_s_vD).first][i] == -1){fErrorOnLambda[(*it_s_vD).first] = true;}
			LambdaSum += LambdaArray[(*it_s_vD).first][i]; 
		}
	}
	
	for( it_s_B = fErrorOnLambda.begin();  it_s_B != fErrorOnLambda.end(); it_s_B++)
	{
		if(fErrorOnLambda[(*it_s_B).first]){FuelCanBeBuilt = false;}
	}

	if(FuelCanBeBuilt && LambdaSum > 0 )
	{
		DBGV("Building process from initial stocks succeeded : ")
		IsotopicVector IV 	= BuildFuelFromEqModel(LambdaArray);
		EvolutionData EvolDB 	= FuelType.GenerateEvolutionData(GetDecay(IV,fCycleTime), R_CycleTime, R_Power);

		{
			pair<map<int, IsotopicVector>::iterator, bool> IResult;
			IResult = fReactorFuturIV.insert( pair<int, IsotopicVector>(ReactorId, IV) );
			if(!IResult.second)
				IResult.first->second = IV;
		}
		{
			pair<map<int, EvolutionData>::iterator, bool> IResult;
			IResult = fReactorFuturDB.insert( pair<int, EvolutionData>(ReactorId,EvolDB) );
			if(!IResult.second)
				IResult.first->second = EvolDB;
		}
		fInsideIV += IV;
		AddCumulativeIVIn(IV);
		DBGL
		return;
	}
	else
	{
		DBGV("Building process from initial stocks failed: ")
		//If the building fails : possibility to take a materials from an infinite stock composed of a substitution IV defined by user
		bool IsSubstitutionMaterials = false;

		for( it_s_B = fErrorOnLambda.begin();  it_s_B != fErrorOnLambda.end(); it_s_B++)
		{
			if(fErrorOnLambda[(*it_s_B).first] && fSubstitutionMaterialFromIV[(*it_s_B).first])
				IsSubstitutionMaterials = true;
			else if(fErrorOnLambda[(*it_s_B).first] && !fSubstitutionMaterialFromIV[(*it_s_B).first])
				IsSubstitutionMaterials = false;
		}

		if(IsSubstitutionMaterials)
		{
			DBGV("Using substitute : -> From infinite substitutions IV ")
			
			//Make the user specified composition to decay the fabrication time
	
			map < string , IsotopicVector> CooledSeparatedIV;  

			for( it_s_B = fSubstitutionMaterialFromIV.begin();  it_s_B != fSubstitutionMaterialFromIV.end(); it_s_B++)
				CooledSeparatedIV[(*it_s_B).first] = GetDecay(fSubstitutionIV[(*it_s_B).first], GetCycleTime());
						
			for( it_s_vIV = fStreamArray.begin();  it_s_vIV != fStreamArray.end(); it_s_vIV++)
			{
				if(fErrorOnLambda[(*it_s_vIV).first])
				{
					fStreamArray[(*it_s_vIV).first].clear();
					fStreamArray[(*it_s_vIV).first].push_back(CooledSeparatedIV[(*it_s_vIV).first]);				
				}
			}
			
			//Building the fuel :
			for( it_s_vD = LambdaArray.begin();  it_s_vD != LambdaArray.end(); it_s_vD++)
				LambdaArray[(*it_s_vD).first].clear();

			LambdaArray 			= FuelType.GetEquivalenceModel()->BuildFuel(R_BU, R_HM_Mass, fStreamArray);
			IsotopicVector IV 		= BuildFuelFromEqModel(LambdaArray);

			//Generating the EvolutionData
			EvolutionData EvolDB = FuelType.GenerateEvolutionData(GetDecay(IV,fCycleTime), R_CycleTime, R_Power);
			{	
				pair<map<int, IsotopicVector>::iterator, bool> IResult;
				IResult = fReactorFuturIV.insert( pair<int, IsotopicVector>(ReactorId, IV) );
				if(!IResult.second)
					IResult.first->second = IV;
			}
			{
				pair<map<int, EvolutionData>::iterator, bool> IResult;
				IResult = fReactorFuturDB.insert( pair<int, EvolutionData>(ReactorId,EvolDB) );
				if(!IResult.second)
					IResult.first->second = EvolDB;
			}
			GetParc()->AddOutIncome(IV);
			fInsideIV += IV;
			AddCumulativeIVIn(IV);
			DBGL
		}
		else if(fSubstitutionFuel)
		{
			DBGV("Using substitute : -> From a fixed data base")
			IsotopicVector IV 		= fSubstitutionEvolutionData.GetIsotopicVectorAt(0);
			EvolutionData evolutiondb 	= fSubstitutionEvolutionData * R_HM_Mass / IV.GetTotalMass();

			IV = IV* R_HM_Mass / IV.GetTotalMass();
			{
				pair<map<int, IsotopicVector>::iterator, bool> IResult;
				IResult = fReactorFuturIV.insert( pair<int, IsotopicVector>(ReactorId, IV) );
				if(!IResult.second)
					IResult.first->second = IV;
			}
			{
				pair<map<int, EvolutionData>::iterator, bool> IResult;
				IResult = fReactorFuturDB.insert( pair<int, EvolutionData>(ReactorId,evolutiondb) );
				if(!IResult.second)
					IResult.first->second = evolutiondb;
			}
			GetParc()->AddOutIncome( IV );
			fInsideIV += IV;
			AddCumulativeIVIn(IV);
		}
		else 
		{
			DBGV("No Alternative Solution. Reactor not loaded. ")
			{
				EvolutionData EmptyDB;
				pair<map<int, EvolutionData>::iterator, bool> IResult;
				IResult = fReactorFuturDB.insert( pair<int, EvolutionData>(ReactorId,EmptyDB) );
				if(!IResult.second)
					IResult.first->second = EmptyDB;
			}
			{
				IsotopicVector EmptyIV;
				pair<map<int, IsotopicVector>::iterator, bool> IResult;
				IResult = fReactorFuturIV.insert( pair<int, IsotopicVector>(ReactorId,EmptyIV) );
				if(!IResult.second)
					IResult.first->second = EmptyIV;
			}	
		}

		DBGL
		ResetArrays();

		return;
	}
DBGL
}

void FabricationPlant::BuildArray(int ReactorId)
{
DBGL
	double R_HM_Mass	= GetParc()->GetReactor()[ ReactorId ]->GetHeavyMetalMass();
	vector <IsotopicVector>  StreamArray;			
	vector <cSecond> 	     StreamArrayTime;		
	vector < pair<int,int> >  StreamArrayAdress;
	
	map < string , IsotopicVector>::iterator it;
	
	for( it = fStreamList.begin();  it != fStreamList.end(); it++)
	{	
		if(fInfiniteMaterialFromList[(*it).first]) 
		{
			IsotopicVector IV = (*it).second / (*it).second.GetTotalMass() * R_HM_Mass;
			StreamArray.push_back(IV);
			StreamArrayAdress.push_back(pair<int,int>(0,0));
			StreamArrayTime.push_back(0);
		}
		else
		{
			for(int j = 0; j < (int)fStorage[(*it).first].size(); j++)
			{
				vector<IsotopicVector> IVArray = fStorage[(*it).first][j]->GetIVArray();
				for(int k = 0; k < (int)IVArray.size(); k++)
				{
					IsotopicVector SeparatedIV = Separation(IVArray[k], (*it).second).first;
					if(Norme(SeparatedIV) != 0)
					{
						IsotopicVector CooledSeparatedIV = GetDecay( SeparatedIV , GetCycleTime());
						StreamArray.push_back( CooledSeparatedIV );
						StreamArrayAdress.push_back( pair<int,int>(j,k) );
						StreamArrayTime.push_back(fStorage[(*it).first][j]->GetIVArrayArrivalTime()[k]);
					}
				}
			}
			SortArray(StreamArray, StreamArrayTime, StreamArrayAdress);
		}

		fStreamArray[(*it).first] 	= StreamArray;		StreamArray.clear();
		fStreamArrayAdress[(*it).first]	= StreamArrayAdress;		StreamArrayTime.clear();
		fStreamArrayTime[(*it).first]	= StreamArrayTime; 		StreamArrayAdress.clear();
	}
DBGL
}

void FabricationPlant::SortArray(vector <IsotopicVector>  StreamArray, vector <cSecond>  StreamArrayTime, vector < pair<int,int> >  StreamArrayAdress)
{
	vector<IsotopicVector>	IVArray;
	vector<cSecond>		TimeArray;
	vector< pair<int,int> >	AdressArray;

	IVArray		= StreamArray;
	TimeArray	= StreamArrayTime;
	AdressArray	= StreamArrayAdress;

	if(fFiFo)
	{
		for(int j = 0; j < (int)TimeArray.size(); j++)
		{
			for (int k = j+1; k < (int)TimeArray.size(); k++)
			{
				cSecond time_tmp = TimeArray[j];
				pair<int,int> Adress_tmp = AdressArray[j];
				IsotopicVector IV_tmp = IVArray[j];

				if(time_tmp > TimeArray[k])
				{
					TimeArray[j] = TimeArray[k];
					TimeArray[k] = time_tmp;

					AdressArray[j] = AdressArray[k];
					AdressArray[k] = Adress_tmp;

					IVArray[j] = IVArray[k];
					IVArray[k] = IV_tmp;
				}
			}
		}
	}
	else
	{
		for(int j = 0; j < (int)TimeArray.size(); j++)
		{
			for (int k = j+1; k < (int)TimeArray.size(); k++)
			{
				cSecond time_tmp = TimeArray[j];
				pair<int,int> Adress_tmp = AdressArray[j];
				IsotopicVector IV_tmp = IVArray[j];

				if(time_tmp < TimeArray[k])
				{
					TimeArray[j] = TimeArray[k];
					TimeArray[k] = time_tmp;

					AdressArray[j] = AdressArray[k];
					AdressArray[k] = Adress_tmp;

					IVArray[j] = IVArray[k];
					IVArray[k] = IV_tmp;
				}	
			}
		}
	}


	StreamArray		= IVArray;
	StreamArrayTime	= TimeArray;
	StreamArrayAdress	= AdressArray;

}

//________________________________________________________________________
//	Substitution Fuel
//________________________________________________________________________

void FabricationPlant::SetSubstitutionFuel(EvolutionData fuel)
{
	
	fSubstitutionFuel = true;

	double M0 = cZAIMass.GetMass( fuel.GetIsotopicVectorAt(0.).GetActinidesComposition() );
	fSubstitutionEvolutionData = fuel / M0;

}


//________________________________________________________________________
//_____________________________ Reactor & DB _____________________________
void FabricationPlant::TakeReactorFuel(int Id)
{
DBGL
	IsotopicVector IV;
	map<int ,IsotopicVector >::iterator it2 = fReactorFuturIV.find( Id );

	AddCumulativeIVOut(it2->second);

	if (it2 != fReactorFuturIV.end())
		(*it2).second = IV;

	map< int,EvolutionData >::iterator it = fReactorFuturDB.find(Id);
	(*it).second = EvolutionData();

	UpdateInsideIV();
DBGL
}

//________________________________________________________________________
EvolutionData FabricationPlant::GetReactorEvolutionDB(int ReactorId)
{

	map< int,EvolutionData >::iterator it = fReactorFuturDB.find(ReactorId);
	return (*it).second;
}
//________________________________________________________________________
//_______________________________ Storage ________________________________
//________________________________________________________________________
IsotopicVector FabricationPlant::BuildFuelFromEqModel(map <string , vector<double> > LambdaArray)
{
DBGL
	IsotopicVector BuildedFuel;
	IsotopicVector Lost;
		
	map < string , IsotopicVector>::iterator it;

	for( it = fStreamList.begin();  it != fStreamList.end(); it++)
	{	
		for(int i = 0; i < (int)fStreamArray[(*it).first].size(); i++)
		{
			if(fInfiniteMaterialFromList[(*it).first])
				BuildedFuel += fStreamArray[(*it).first][i]*LambdaArray[(*it).first][i];

			else if (fSubstitutionMaterialFromIV[(*it).first] && fErrorOnLambda[(*it).first])
				BuildedFuel += fStreamArray[(*it).first][i]*LambdaArray[(*it).first][i];	
			else
			{
				if(LambdaArray[(*it).first][i] != 0)
				{
					int Stor_N 	= fStreamArrayAdress[(*it).first][i].first;
					int IV_N 	= fStreamArrayAdress[(*it).first][i].second;
	
					pair<IsotopicVector, IsotopicVector> Separated_Lost;
					Separated_Lost = Separation( fStorage[(*it).first][Stor_N]->GetIVArray()[IV_N]*LambdaArray[(*it).first][i], (*it).second );
					BuildedFuel += Separated_Lost.first;
					Lost += Separated_Lost.second;
					
				}
			}
		}
	}

	if(fIsReusable)
		fReUsable->AddIV(Lost);
	else
		GetParc()->AddWaste(Lost);

	DumpStock(LambdaArray);

DBGL
	return BuildedFuel;
}

//________________________________________________________________________
void FabricationPlant::DumpStock(map <string , vector<double> > LambdaArray)
{
DBGL

	map < string , IsotopicVector>::iterator it;

	for( it = fStreamList.begin();  it != fStreamList.end(); it++)
	{	
		for(int i = 0; i < (int)fStreamArray[(*it).first].size(); i++)
		{
			if(fInfiniteMaterialFromList[(*it).first])
			{
				GetParc()->AddOutIncome( fStreamArray[(*it).first][0]*LambdaArray[(*it).first][i] );
			}	
			
			else
			{		
				if(LambdaArray[(*it).first][i] != 0)
				{
					int Stor_N = fStreamArrayAdress[(*it).first][i].first;
					int IV_N = fStreamArrayAdress[(*it).first][i].second;
					fStorage[(*it).first][Stor_N]->TakeFractionFromStock( IV_N, LambdaArray[(*it).first][i] );
				}
			}
		}
	}

	ResetArrays();

DBGL
}
//________________________________________________________________________
void FabricationPlant::ResetArrays()
{
		//Clear the Building Array 
	map < string , IsotopicVector>::iterator it;

	for( it = fStreamList.begin();  it != fStreamList.end(); it++)
	{		
		fStreamList[(*it).first]= IsotopicVector();
		fStreamArray[(*it).first].clear();
		fStreamArrayTime[(*it).first].clear();
		fStreamArrayAdress[(*it).first].clear();

	}


}
//________________________________________________________________________
pair<IsotopicVector, IsotopicVector> FabricationPlant::Separation(IsotopicVector isotopicvector, IsotopicVector ExtractedList)
{
DBGL
		//[0] = re-use ; [1] = waste
	IsotopicVector LostInReprocessing  	= isotopicvector.GetThisComposition(ExtractedList) * fSeparationLostFraction;
	IsotopicVector SeparatedPart  	= isotopicvector.GetThisComposition(ExtractedList) - LostInReprocessing;
	IsotopicVector LostPart		= isotopicvector - SeparatedPart;
DBGL
	return pair<IsotopicVector, IsotopicVector> (SeparatedPart, LostPart);
}


//________________________________________________________________________
//	Get Decay
//________________________________________________________________________
IsotopicVector FabricationPlant::GetDecay(IsotopicVector isotopicvector, cSecond t)
{

	IsotopicVector IV;

	map<ZAI ,double> isotopicquantity = isotopicvector.GetIsotopicQuantity();
	map<ZAI ,double >::iterator it;
	for( it = isotopicquantity.begin(); it != isotopicquantity.end(); it++)
	{
		if((*it).second > 0)
		{
 			IsotopicVector ivtmp = fDecayDataBase->Evolution(it->first, t) * (*it).second ;
			IV += ivtmp;
		}
	}

	return IV;
	
}

void FabricationPlant::AddInfiniteStorage(string keyword)
{
	Storage* Stock;
	fStorage[keyword].push_back(Stock);
	fInfiniteMaterialFromList[keyword] = true;
} 
