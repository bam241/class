#include "ZAI.hxx"
#include "CLASSHeaders.hxx"


//const string DEFAULTDATABASE = "DecayBase.dat";
//________________________________________________________________________
//
//		ZAI
//
//
//
//
//________________________________________________________________________
//____________________________InClass Operator____________________________
//________________________________________________________________________
ClassImp(ZAI)

ZAI ZAI::operator=(ZAI IVa)
{
	fZ = IVa.Z();
	fA = IVa.A();
	fI = IVa.I();
	return *this;
}



ZAI::ZAI()
{
		
	fName="";
	fZ=0;
	fA=0;
	fI=0;

}


//________________________________________________________________________
ZAI::ZAI(int Z, int A, int I)
{

	fZ=Z;
	fA=A;
	fI=I;

}


ZAI::~ZAI()
{
		
}


