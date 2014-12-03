#ifndef _CLASSNucleiFiliation_
#define _CLASSNucleiFiliation_

/*!
 \file
 \brief Header file for CLASSNucleiFiliation classes.
 */

#include "CLASSObject.hxx"
#include "IsotopicVector.hxx"

using namespace std;

//-----------------------------------------------------------------------------//
/*!
 Define a nuclei as : Z A I.
 The aim of this class is to discribe each CLASSNucleiFiliation.
 
 @author BaM
 @version 2.0
 */
//________________________________________________________________________



class CLASSNucleiFiliation : public CLASSObject
{
public:
	
	
	//********* Constructor/Destructor Method *********//
	
	/*!
	 \name Constructor/Desctructor
	 */
	//@{
	
	CLASSNucleiFiliation();	///< Default constructor
	
	CLASSNucleiFiliation(CLASSNucleiFiliation CNF); ///< Copy Constructor
	
	
	~CLASSNucleiFiliation();	///< Normal Destructor.
	
	//}

	
	//********* Get Method *********//
	
	/*!
	 \name Get Method
	 */
	//@{
	map<ZAI, IsotopicVector> GetNucleiFIliation() const {return fNucleiFiliation;}

	IsotopicVector GetFiliation(ZAI Mother);
	
	//}
	
	//********* Add Method *********//
	
	/*!
	 \name Adding Method
	 */
	//@{
	
	
	void AddDaughterToZAI(ZAI Mother, IsotopicVector Daughter );
	
	//}


	//********* Modification Method *********//
	
	/*!
	 \name Modification Method
	 */
	//@{
	
	
	void FiliationCleanUp(map<ZAI, int> GoodNuclei, CLASSNucleiFiliation CuttedNuclei);

	void SelfFiliationCleanUp(map<ZAI, int> GoodNuclei);
	
	
	

	void NormalizeBranchingRatio(double Value = 1);

	void NormalizeBranchingRatio(ZAI Mother, double Value);

	//}
	
	
protected :
	
	map<ZAI, IsotopicVector> fNucleiFiliation;
	
	
	ClassDef(CLASSNucleiFiliation,1);
};


#endif
