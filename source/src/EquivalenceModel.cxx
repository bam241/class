#include "EquivalenceModel.hxx"
#include "external/StringLine.hxx"
#include "CLASSMethod.hxx"

//________________________________________________________________________
EquivalenceModel::EquivalenceModel():CLASSObject()
{
	fRelativMassPrecision 	= 5/10000.; 	//Mass precision
	fMaxIterration 		= 500; 		// Max iterration in build fueld algorythum
	freaded 		= false;
	
	EquivalenceModel::LoadKeyword();
}
//________________________________________________________________________
EquivalenceModel::EquivalenceModel(CLASSLogger* log):CLASSObject(log)
{
	fRelativMassPrecision 	= 5/10000.; 	//Mass precision
	fMaxIterration 		= 500; 		// Max iterration in build fueld algorythm
	freaded 		= false;
	
	EquivalenceModel::LoadKeyword();
}
//________________________________________________________________________
EquivalenceModel::~EquivalenceModel()
{

}
//________________________________________________________________________
void EquivalenceModel::ReadNFO()
{
	DBGL
	ifstream NFO(fInformationFile.c_str());
	
	if(!NFO)
	{
		ERROR << "Can't find/open file " << fInformationFile << endl;
		exit(0);
	}
	
	do
	{
		string line;
		getline(NFO,line);
		
		EquivalenceModel::ReadLine(line);
		
	} while(!NFO.eof());
	
	DBGL
}
//________________________________________________________________________
void EquivalenceModel::ReadLine(string line)
{
	DBGL
	
	if (!freaded)
	{
		int pos = 0;
		string keyword = tlc(StringLine::NextWord(line, pos, ' '));
		
		map<string, EQM_MthPtr>::iterator it = fKeyword.find(keyword);
		
		if(it != fKeyword.end())
			(this->*(it->second))( line );
		
		freaded = true;
		ReadLine(line);
		
	}
	
	freaded = false;
	
	DBGL
}
//________________________________________________________________________
void EquivalenceModel::LoadKeyword()
{
	DBGL
	fKeyword.insert( pair<string, EQM_MthPtr>( "k_zail",			& EquivalenceModel::ReadZAIlimits)	 	 );
	fKeyword.insert( pair<string, EQM_MthPtr>( "k_reactor",			& EquivalenceModel::ReadType)	 	 );
	fKeyword.insert( pair<string, EQM_MthPtr>( "k_fuel",			& EquivalenceModel::ReadType)	 	 );
	fKeyword.insert( pair<string, EQM_MthPtr>( "k_firstguesscontent",	& EquivalenceModel::ReadFirstGuessContent) 	 );
	fKeyword.insert( pair<string, EQM_MthPtr>( "k_list",			& EquivalenceModel::ReadList) 	 		 );
	fKeyword.insert( pair<string, EQM_MthPtr>( "k_specpower",		& EquivalenceModel::ReadSpecificPower)	 );
	DBGL
}
//________________________________________________________________________
void EquivalenceModel::PrintInfo()
{
	INFO << "Reactor Type : "<< fDBRType << endl;
	INFO << "Fuel Type : "<< fDBFType << endl;
	INFO << "Specific Power [W/g]: "<< fSpecificPower << endl;
	
	map < string , IsotopicVector >::iterator it_s_IV;
	map < string , double >::iterator it_s_D;

	for(  it_s_IV = fStreamList.begin();   it_s_IV != fStreamList.end();  it_s_IV++)
	{	
		INFO <<(* it_s_IV).first<<"  (Z A I) :" << endl;
		map<ZAI ,double >::iterator it1;
		map<ZAI ,double > fMap1 = fStreamList[(* it_s_IV).first].GetIsotopicQuantity();
		for(it1 = fMap1.begin()  ; it1 != fMap1.end() ; it1++)
			INFO << (*it1).first.Z() <<" "<< (*it1).first.A() <<" "<< (*it1).first.I() << endl;
	}
	INFO<<"First guess content in fuel : "<<endl;
	for(  it_s_D = fFirstGuessContent.begin();   it_s_D != fFirstGuessContent.end();  it_s_D++)
	{
		INFO <<(* it_s_D).first<<" "<<fFirstGuessContent[(* it_s_D).first]<<endl;
	}	

	INFO<<"ZAI limits (validity domain)[prop in fresh fuel] (Z A I min max) :"<<endl;
	for (map< ZAI,pair<double,double> >::iterator Domain_it = fZAILimits.begin(); Domain_it != fZAILimits.end(); Domain_it++)
	{	
		double ThatZAIMin  = Domain_it->second.first;
		double ThatZAIMax  = Domain_it->second.second;
		int Z = Domain_it->first.Z();
		int A = Domain_it->first.A();
		int I = Domain_it->first.I();

		INFO <<ThatZAIMin<<" < ZAI ("<< Z<< " " << A << " " << I<<")"<<" < "<<ThatZAIMax<< endl;
	}

}
//________________________________________________________________________
void EquivalenceModel::ReadType(const string &line)
{
	DBGL
	int pos = 0;
	string keyword = tlc(StringLine::NextWord(line, pos, ' '));
	if( keyword != "k_fuel" && keyword != "k_reactor" )	// Check the keyword
	{
		ERROR << " Bad keyword : " << keyword << " Not found !" << endl;
		exit(1);
	}
	if( keyword ==  "k_fuel" )
		fDBFType = StringLine::NextWord(line, pos, ' ');
	else if( keyword ==  "k_reactor" )
		fDBRType = StringLine::NextWord(line, pos, ' ');
	
	DBGL
}
//________________________________________________________________________
void EquivalenceModel::ReadZAIlimits(const string &line)
{
	DBGL
	int pos = 0;
	string keyword = tlc(StringLine::NextWord(line, pos, ' '));
	if( keyword != "k_zail" )	// Check the keyword
	{
		ERROR << " Bad keyword : \"k_zail\" not found !" << endl;
		exit(1);
	}
	
	int Z 	= atoi(StringLine::NextWord(line, pos, ' ').c_str());
	int A 	= atoi(StringLine::NextWord(line, pos, ' ').c_str());
	int I 	= atoi(StringLine::NextWord(line, pos, ' ').c_str());
	
	double downLimit 	= atof(StringLine::NextWord(line, pos, ' ').c_str());
	double upLimit 		= atof(StringLine::NextWord(line, pos, ' ').c_str());
	
	if (upLimit < downLimit)
	{
		double tmp 	= upLimit;
		upLimit 	= downLimit;
		downLimit 	= tmp;
	}
	fZAILimits.insert(pair<ZAI, pair<double, double> >(ZAI(Z,A,I), pair<double,double>(downLimit, upLimit)));
	DBGL
}
//________________________________________________________________________
void EquivalenceModel::ReadList(const string &line)
{
	DBGL
	int pos = 0;
	string keyword = tlc(StringLine::NextWord(line, pos, ' '));
	if( keyword != "k_list" )	// Check the keyword
	{
		ERROR << " Bad keyword : \"k_list\" not found !" << endl;
		exit(1);
	}
	string ListName	= StringLine::NextWord(line, pos, ' ');
	int Z 		= atoi(StringLine::NextWord(line, pos, ' ').c_str());
	int A 		= atoi(StringLine::NextWord(line, pos, ' ').c_str());
	int I 		= atoi(StringLine::NextWord(line, pos, ' ').c_str());
	double Q 	= atof(StringLine::NextWord(line, pos, ' ').c_str());
	fStreamList[ListName].Add(Z, A, I, Q);
	
	DBGL
}
//________________________________________________________________________
void EquivalenceModel::ReadFirstGuessContent(const string &line)
{
	DBGL
	int pos = 0;
	string keyword = tlc(StringLine::NextWord(line, pos, ' '));
	if( keyword != "k_firstguesscontent" )	// Check the keyword
	{
		ERROR << " Bad keyword : \"k_firstguesscontent\" not found !" << endl;
		exit(1);
	}
	string ListName	= StringLine::NextWord(line, pos, ' ');
	double Q 	= atof(StringLine::NextWord(line, pos, ' ').c_str());
	fFirstGuessContent[ListName] = Q;
	
	DBGL
}
//________________________________________________________________________
void EquivalenceModel::ReadSpecificPower(const string &line)
{
	DBGL
	int pos = 0;
	string keyword = tlc(StringLine::NextWord(line, pos, ' '));
	if( keyword != "k_specpower")	// Check the keyword
	{
		ERROR << " Bad keyword : \"k_specpower\" Not found !" << endl;
		exit(1);
	}
	
	fSpecificPower = atof(StringLine::NextWord(line, pos, ' ').c_str());
	
	DBGL
}

//________________________________________________________________________
void EquivalenceModel::StocksTotalMassCalculation(map < string , vector <IsotopicVector> > const& Stocks)
{
	// Calculating total mass of stock once and for all
	
	
	double TotalMassInStocks = 0;
	map < string , vector <IsotopicVector> >::const_iterator it_s_vIV;

	for( it_s_vIV = Stocks.begin();  it_s_vIV != Stocks.end(); it_s_vIV++)
	{
		fTotalMassInStocks[ it_s_vIV->first ] = 0;
		fLambdaMax[ it_s_vIV->first ] = 0;
	}

	for(  it_s_vIV = Stocks.begin();   it_s_vIV != Stocks.end();  it_s_vIV++)
	{	
		TotalMassInStocks = 0;
		for(int i=0; i < (int)Stocks.at((* it_s_vIV).first).size(); i++)
		{
			TotalMassInStocks  +=  Stocks.at( it_s_vIV->first )[i].GetTotalMass();
		}
		fLambdaMax[(*it_s_vIV).first] = Stocks.at( it_s_vIV->first ).size();	
		fTotalMassInStocks[ it_s_vIV->first ] = TotalMassInStocks * 1e6; // in grams
	}
}

//________________________________________________________________________
double EquivalenceModel::LambdaCalculation(string MaterialDenomination, double LambdaPreviousStep, double MaterialMassNeeded, double DeltaMass, vector <IsotopicVector>  Stocks)
{
	double Lambda_tot = 0; 

	// If there is not enough matter in stocks construction fails
	if( MaterialMassNeeded > fTotalMassInStocks[MaterialDenomination] )
	{
		if(DeltaMass > 0)
			Lambda_tot = LambdaPreviousStep - (fLambdaMax[MaterialDenomination] - LambdaPreviousStep)/2 ; 
		
		if(DeltaMass < 0)
			Lambda_tot = LambdaPreviousStep + (fLambdaMax[MaterialDenomination] - LambdaPreviousStep)/2 ; 
		
		if( (fLambdaMax[MaterialDenomination] - Lambda_tot)<1 && (fLambdaMax[MaterialDenomination]-Lambda_tot)*Stocks.back().GetTotalMass()*1e6<MaterialMassNeeded*fRelativMassPrecision/2.)
		{
			WARNING << "Not enough " << MaterialDenomination << " material to build fuel" << endl;
			WARNING << "Mass available "<<fTotalMassInStocks[MaterialDenomination] << endl;
			WARNING << "Mass needed "<<MaterialMassNeeded<< endl;
			Lambda_tot = -1;
		}
	}
	
	// Lambda calculation
	else
	{
		for( int i=0; i < (int)Stocks.size(); i++)
		{	
			if( MaterialMassNeeded >= (Stocks[i].GetTotalMass()*1e6))
			{
				Lambda_tot +=  1;
				MaterialMassNeeded -=  (Stocks[i].GetTotalMass()*1e6);
			}
			else
			{
				Lambda_tot +=  MaterialMassNeeded/(Stocks[i].GetTotalMass()*1e6);
				break;
			}
		}
	}

	return Lambda_tot;
}
//________________________________________________________________________
void EquivalenceModel::SetLambda(vector<double>& lambda, double Lambda_tot)
{
	if(Lambda_tot > (int)lambda.size() )
	{
		cout<<Lambda_tot<<"  "<<lambda.size()<<endl;
		ERROR << " FATAL ERROR " <<endl;
		exit(0);
	}

	for(int i = 0 ; i < (int)lambda.size() ; i++) //set to 0 all non touched value (to be sure)
		lambda[i] = 0  ;

	int IntegerPart 		= floor( Lambda_tot );
	double DecimalPart 	= Lambda_tot - IntegerPart;

	for(int i=0  ; i < IntegerPart; i++ )
		lambda[i]=1;

	lambda[IntegerPart] = DecimalPart;
}

//________________________________________________________________________
void EquivalenceModel::SetLambdaToErrorCode(vector<double>& lambda)
{

	if(lambda.size() == 0) //then we have to add an element to send the error code to the fab (case for no storage in stream)
	{
		lambda.push_back(-1);
	}	

	else // other errors (no enough material or too many steps)
	{
		for( int i=0; i < (int)lambda.size(); i++)
		{	
			lambda[i] = -1;	
		}
	}	

}

//________________________________________________________________________
map <string , vector<double> > EquivalenceModel::BuildFuel(double BurnUp, double HMMass, map < string , vector <IsotopicVector> > StreamArray,  map < string , double> StreamListMassFractionMin, map < string , double> StreamListMassFractionMax, map < string , int> StreamListPriority, map < string , bool> StreamListIsBuffer)
{
DBGL

	map <string , vector<double> > lambda ; // map containing name of the list and associated vector of proportions taken from stocks
	
	//Iterators declaration
	map < string , vector  <IsotopicVector> >::iterator it_s_vIV;
	map < string , vector  <double> >::iterator it_s_vD;
	map < string , IsotopicVector >::iterator it_s_IV;
	map < string , double >::iterator it_s_D;
	map < string , bool >::iterator it_s_B;
	
	for( it_s_vIV = StreamArray.begin();  it_s_vIV != StreamArray.end(); it_s_vIV++)
	{	
		for(int i=0; i < (int)StreamArray[(*it_s_vIV).first].size(); i++)
		{
			lambda[(*it_s_vIV).first].push_back(0);
		}
	}	

	/*** Test if there is at least one stock available in each list, otherwise fuel is not built ***/
	bool BreakReturnLambda = false; 
	for( it_s_vIV = StreamArray.begin();  it_s_vIV != StreamArray.end(); it_s_vIV++)
	{
		if(StreamArray[(*it_s_vIV).first].size() == 0)
		{
			WARNING << " No stock available for stream : "<< (*it_s_vIV).first <<".  Fuel not built." << endl;
			SetLambdaToErrorCode(lambda[(*it_s_vIV).first]);
			BreakReturnLambda = true; 	
		}
	}
	if(BreakReturnLambda) { return lambda;}
	HMMass *=  1e6; //Unit onversion : tons to gram
	
	/**** Build a stream array containing IVs of each material in the limits imposed by the user, the model or the available stocks ***/

	map <string, vector < IsotopicVector > > SortedStreamArray = BuildSortedStreamArray (StreamArray, StreamListMassFractionMin,  StreamListMassFractionMax,  StreamListPriority,  StreamListIsBuffer) ; 

	vector < double > BurnUpAsAFonctionOfMass; 
	vector < double > MassOfAvailableMaterial ; 

	bool BurnUpExceeded 		= false ;
	int BurnUpExceededPosition 	= 0;
	double HigherLimitOnBurnUp	= 0;
	int StreamListNumber = 0;

	/**** Search in the sorted stream array the point where calculated BU is higher than targeted BU***/
	IsotopicVector FuelToTestWithoutBuffer = IsotopicVector();
	IsotopicVector PreviousFuelToTestWithoutBuffer = IsotopicVector();
	for( it_s_vIV = SortedStreamArray.begin();  it_s_vIV != SortedStreamArray.end(); it_s_vIV++)
	{	
		if(!BurnUpExceeded)
		{
			if(!StreamListIsBuffer[(*it_s_vIV).first])
			{	
				for(int i=0; i < (int)SortedStreamArray[(*it_s_vIV).first].size(); i++)
				{
					PreviousFuelToTestWithoutBuffer = FuelToTestWithoutBuffer ; //keep in memory fuel test during last step. When Burn-up is exceeded it will be the starting point.
					FuelToTestWithoutBuffer      +	= SortedStreamArray[(*it_s_vIV).first][i] ;
					IsotopicVector Buffer 		= BuildBuffer(FuelToTestWithoutBuffer , HMMass, SortedStreamArray) ;
					IsotopicVector FuelToTest 	= FuelToTestWithoutBuffer + Buffer ; 
					FuelToTest 			= FuelToTest/FuelToTest.GetSumOfAll();
					double EqMMaximumBurnUp	= GetMaximumBurnUp (FuelToTest, BurnUp) ;
	
					BurnUpAsAFonctionOfMass.push_back(EqMMaximumBurnUp);
					MassOfAvailableMaterial.push_back(FuelToTestWithoutBuffer.GetTotalMass());
					if(EqMMaximumBurnUp>BurnUp)
					{
						BurnUpExceeded 		= true;
						BurnUpExceededPosition	= i;
						HigherLimitOnBurnUp		= EqMMaximumBurnUp;
						break;
					}
				}
			}
		}
		else
		{
			break;
		}
		StreamListNumber ++;
	}
	if (StreamListNumber == 0 && BurnUpExceededPosition==0)
	{
		WARNING << " Lower limit of first priority material is already to high for the target Burn-Up. Lower limit should be decreased.  Fuel not built." << endl;
		SetLambdaToErrorCode(lambda[(*it_s_vIV).first]);
		return lambda;	
	}
	double FractionOfLastIVToAdd 		= 1.0; //Start with 100% of the last IV in the fuel
	double MaxFractionOfLastIVToAdd 		= 1.0;
	double MinFractionOfLastIVToAdd 		= 0.0;
	
	FuelToTestWithoutBuffer  			= PreviousFuelToTestWithoutBuffer ; // Fuel tested at the step before Burn-up targeted is exceeded.
	double CalculatedBurnUp 			= HigherLimitOnBurnUp ; 

	double LastFractionOfLastIVToAddMinus 	= 0.0;
	double LastFractionOfLastIVToAddPlus	= 0.0;	

	int count = 0;
	
	/**** Search in the fraction of last IV to add to the fuel to reach BU***/

	do
	{
		if(count > fMaxIterration)
		{
			ERROR << "CRITICAL ! Can't manage to predict fissile content\nHint : Try to decrease the precision on burnup using :\nYourEquivalenceModel->SetBurnUpPrecision(Precision); " << endl;
			ERROR << "Targeted Burnup :" <<BurnUp<<endl;
			ERROR << "Last calculated Burnup :" <<CalculatedBurnUp<<endl;
			ERROR << "Last Fresh fuel composition without buffer:" <<endl;
			ERROR << FuelToTestWithoutBuffer .sPrint()<<endl;
			
			exit(1);
		}

		if( (CalculatedBurnUp - BurnUp) < 0 ) //Need to add more of last IV in fuel
		{
			LastFractionOfLastIVToAddMinus 	= 
			FractionOfLastIVToAdd 		= FractionOfLastIVToAdd + fabs(LastFractionOfLastIVToAddPlus - FractionOfLastIVToAdd)/2.;
		}
		else if( (CalculatedBurnUp - BurnUp) > 0) //Need to add less of last IV in fuel
		{
			LastFractionOfLastIVToAddPlus 	=
			FractionOfLastIVToAdd 		= FractionOfLastIVToAdd - fabs(LastFractionOfLastIVToAddMinus - FractionOfLastIVToAdd)/2.;
		}

		count ++;

	}while(fabs(BurnUp - CalculatedBurnUp) > GetBurnUpPrecision()*BurnUp);

	lambda = FromSortedArrayToInitialStreamArray();

	for( it_s_IV = IVStream.begin();  it_s_IV != IVStream.end(); it_s_IV++)
		(*this).isIVInDomain(IVStream[(*it_s_IV).first]);
		
	for( it_s_vD = lambda.begin();  it_s_vD != lambda.end(); it_s_vD++)
	{	
		DBGV( "Lambda vector : "<<(*it_s_vD).first );

		for(int i=0; i < (int)lambda[(*it_s_vD).first].size(); i++)
		{
			DBGV(lambda[(*it_s_vD).first][i]); 
		}
	}		
	return lambda;
}

//________________________________________________________________________
bool EquivalenceModel::isIVInDomain(IsotopicVector IV)
{
	DBGL
	bool IsInDomain = true;
	
	if(fZAILimits.empty())
	{
	 WARNING << "Fresh Fuel variation domain is not set" << endl;
	 WARNING << "CLASS has no clue if the computed evolution for this fresh fuel is correct" << endl;
	 WARNING << "Proceed finger crossed !!" << endl;
	 return true;
	}
	
	else
	{
		IsotopicVector IVNorm = IV /IV.GetSumOfAll();
		for (map< ZAI,pair<double,double> >::iterator Domain_it = fZAILimits.begin(); Domain_it != fZAILimits.end(); Domain_it++)
		{
			double ThatZAIProp = IVNorm.GetIsotopicQuantity()[Domain_it->first];
			double ThatZAIMin  = Domain_it->second.first;
			double ThatZAIMax  = Domain_it->second.second;
			if( (ThatZAIProp > ThatZAIMax) || (ThatZAIProp <  ThatZAIMin) )
			{
				IsInDomain = false;
				
				WARNING << "Fresh fuel out of model range" << endl;
				WARNING << "\t AT LEAST this ZAI is accused to be outrange :" << endl;
				WARNING << "\t\t" << Domain_it->first.Z() << " " << Domain_it->first.A() << " " << Domain_it->first.I() << endl;
				WARNING << "\t\t min = " << ThatZAIMin  << " value = " << ThatZAIProp << " max = " << ThatZAIMax << endl;
				WARNING << "\t IV accused :" << endl << endl;
				WARNING << IVNorm.sPrint() << endl;
				break;
			}
		}
	}
	DBGL
	return IsInDomain;
	
}

