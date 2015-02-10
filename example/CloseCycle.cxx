/************************************************************/
//              DESCRIPTION
// Close Fuel cycle scenario :
// This park is constituted by a  PWR MOX which 
// multi-recycle its own fuel.
// The Storage is initially filled with Pu in order
// to this scenario to be doable
//         _______________	_______     ____    _______
//        |                |   |       |   |    |  |       |
//  ||===>|FabricationPlant| =>|Reactor| =>|Pool|=>|Storage|===||
//  ||    |________________|   |_______|   |____|  |_______|   ||
//  ||=========================================================||
//
// The spent fuel goes to the pool for 5 y
// then it goes to the Storage
//The scenario is run for 40 years
//
//@author BaL
/***********************************************************/
#include "CLASSHeaders.hxx"
#include <sstream>
#include <iomanip>
#include <math.h>
#include <string>
#include "XS/XSM_MLP.hxx"					//Load the include for Neural network cross section predictor 
#include "Irradiation/IM_RK4.hxx"			//Load the include for Runge Kutta 4 resolution
#include "Equivalence/EQM_MLP_PWR_MOX.hxx"	//Load the include for Neural Network Equivalence Model (PWRMOX)
using namespace std;

int main(int argc, char** argv)
{
	//seconds in one year
	cSecond year = 3600*24.*365.25; 
	/******LOG MANAGEMENT**********************************/
	//Definition of the Log file : CLASS messages output 
	int Std_output_level=0;  // Only error are shown in terminal
	int File_output_level=2; // Error + Warning + Info are shown in the file CLASS_OUTPUT.log
	CLASSLogger *Logger	 = new CLASSLogger("CLASS_OUTPUT.log",Std_output_level,File_output_level);

	/******SCENARIO**********************************/
	// The scenario start at year 1977
	Scenario *gCLASS=new Scenario(1977*year,Logger);
	gCLASS->SetStockManagement(true);//If false all the IsotopicVector in stocks are mixed together.
	gCLASS->SetTimeStep(year/4.);	 //the scenario calculation is updated every 3 months
	gCLASS->SetOutputFileName("CloseCycle.root");	//Set the name of the output file

	/******DATA BASES**********************************/
	/*===Decay data base===*/
	//The decay data base is taken from the file Decay.idx
	DecayDataBank* DecayDB = new DecayDataBank(gCLASS->GetLog(), "../DATA_BASES/DECAY/ALL/Decay.idx");
	gCLASS->SetDecayDataBase(DecayDB);//This decay data base will be used for all the decay calculations in this Scenario

	/*===Reactor data base===*/

	XSM_MLP* XSMOX = new XSM_MLP(gCLASS->GetLog(), "../DATA_BASES/PWR/MOX/XSModel/30Wg_FullMOX");//Defining the XS Predictor
	IM_RK4 *IMRK4 = new IM_RK4(gCLASS->GetLog());													  //Bateman's equation solver method (RungeKutta4)
	EQM_MLP_PWR_MOX* EQMLINPWRMOX = new EQM_MLP_PWR_MOX(gCLASS->GetLog(),"../DATA_BASES/PWR/MOX/EQModel/EQM_MLP_PWR_MOX_3batch.xml");//Defining the EquivalenceModel

	PhysicsModels* PHYMOD = new PhysicsModels(XSMOX, EQMLINPWRMOX, IMRK4); 							 //The PhysicsModels containing the 3 object previously defined


	/******FACILITIES*********************************/
	/*===A Stock===*/
	Storage *Stock = new Storage(gCLASS->GetLog()); // Definition of the stock
	Stock->SetName("Stock"); 					// Its name
	//Fill the stock with an initial amount of Plutonium
	// In order to allow the PWR_MOX to work
	IsotopicVector InitialIV;
	InitialIV.Add(94,238,0,4e27  );
	InitialIV.Add(94,239,0,6.4e28);
	InitialIV.Add(94,240,0,1.6e28);
	InitialIV.Add(94,241,0,9.0e27);
	InitialIV.Add(94,242,0,6e27  );
	InitialIV.Add(95,241,0,1e27  );
	gCLASS->Add(Stock); 	//Adding the stock to the Scenario 
	Stock->AddToStock(InitialIV);

	/*===A Pool===*/
	Pool *Cooling_MOX = new Pool(gCLASS->GetLog(),Stock, 5*year); //After 5 years of cooling, the pool sends its content to "Stock"
	Cooling_MOX->SetName("Pool_MOX");
	gCLASS->Add(Cooling_MOX);

	/*===A FabricationPlant===*/
	FabricationPlant *FP_MOX = new FabricationPlant(gCLASS->GetLog(), 3*year); //Declare a FabricationPlant. After the build of the fuel, it decays during 3years before to be loaded in Reactor
	FP_MOX->SetFiFo(false); //The latest isotopicVector to enter in "Stock" will be used to build the fuel (Opposite of First In First Out)
	FP_MOX->SetName("Fab_MOX");
	FP_MOX->AddFissileStorage(Stock);	//Tell the FP to look in Stock for fissionable material 
	//FP_MOX->AddFertileStorage(Stock2);//Tell the FP to look in Stock2 for fertile material 
	//If fertile stock is not defined (like here), CLASS get fertile from nature (OUTCOMING vector)
	//FP_MOX->SetReUsableStorage(wastestock);//By default the fabricationplant get the list of nuclei defined in the EquivalenceModel (here EQM_MLP_MOX) from stock and send the others nuclei in WASTE. If user want these nuclei to go in another stock  he can use the SetReUsableStorage function
	gCLASS->AddFabricationPlant(FP_MOX);


	/*===A Reactor : PWR_UOX===*/
	double  HMMass    = 72.5;		//heavy metal mass (in tons)
	double	Power_CP0 = 2660e6;	    //Thermal power (in W)
	double  BurnUp    = 35; 		//35 GWd/tHM

	cSecond StartingTime =  1985*year;
	cSecond LifeTime     =  40*year;
					
	Reactor* PWR_MOX = new Reactor(gCLASS->GetLog(),// Log
							   PHYMOD,				// The models used to build the fuel & to calculate its evolution
							   FP_MOX,				// The FabricationPlant
							   Cooling_MOX,			// Connected Backend facility : The reactor discharge its fuel into the Pool "Cooling_UOX"
							   StartingTime,		// Starting time
							   LifeTime,			// time of reactor life time
							   Power_CP0,			// Power
							   HMMass,				// HM mass
							   BurnUp,				// BurnUp
							   0.8);				// Load Factor
					

	PWR_MOX->SetName("a_PWR_MOX");// name of the reactor (as it will show up in the CLASSGui)
	gCLASS->AddReactor(PWR_MOX);//Add this reactor to the scenario
					
	gCLASS->Evolution((double)year*2018);//Perform the calculation from year 1977(defined in Scenario declaration) to year 2018

	delete gCLASS;

}


//==========================================================================================
// Compilation
//==========================================================================================
/*
 
 \rm CLASS* ; g++ -o CLASS_Exec CloseCycle.cxx -I $CLASS_include -L $CLASS_lib -lCLASSpkg `root-config --cflags` `root-config --libs` -fopenmp -lgomp -Wunused-result
 
 
 */
