#include "PhysicsModels.hxx"

//________________________________________________________________________
//
//		PhysicsModels
//
//
//
//
//________________________________________________________________________



PhysicsModels::PhysicsModels():CLASSObject()
{

	fXSModel	 = 0;
	fEquivalenceModel = 0;
	fIrradiationModel = 0;


}
//________________________________________________________________________
PhysicsModels::PhysicsModels(XSModel* XS, EquivalenceModel* EM, IrradiationModel* IM ):CLASSObject()
{

	fXSModel	 = XS;
	fEquivalenceModel = EM;
	fIrradiationModel = IM;

	int Z_ZAIThreshold = fIrradiationModel->GetZAIThreshold();
	fXSModel->SetZAIThreshold(Z_ZAIThreshold);


}
//________________________________________________________________________
PhysicsModels::PhysicsModels(CLASSLogger* log, XSModel* XS, EquivalenceModel* EM, IrradiationModel* IM ):CLASSObject(log)
{

	fXSModel	 = XS;
	fEquivalenceModel = EM;
	fIrradiationModel = IM;

	int Z_ZAIThreshold = fIrradiationModel->GetZAIThreshold();
	fXSModel->SetZAIThreshold(Z_ZAIThreshold);


}

//________________________________________________________________________
EvolutionData PhysicsModels::GenerateEvolutionData(IsotopicVector IV, cSecond cycletime, double Power)
{

	fXSModel->isIVInDomain(IV);

	return fIrradiationModel->GenerateEvolutionData(IV, fXSModel->GetCrossSections(IV), Power, cycletime);
}
//________________________________________________________________________


XSModel*		PhysicsModels::GetXSModel()
{
	return fXSModel;
} //!< return the mean cross section predictor

EquivalenceModel*	PhysicsModels::GetEquivalenceModel() {
	return fEquivalenceModel;
} //!< return Fissile content predictor

IrradiationModel*	PhysicsModels::GetIrradiationModel()
{
	return fIrradiationModel;
} //!< return the Bateman solver
