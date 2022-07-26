// -- solve the concentration equation for all chemical species
forAll(specieNames,lstI)
{
    // -- get current name of the specie that is solved for
    word currSpName = specieNames[lstI];                            	

    Info<<"\nWorking on the specie: "<<currSpName<<endl;

    volScalarField& currSp	= MFrList[lstI];                	//current concentration field

    // -- DIFUSION COEFICIENT CALCULATION 
    // -- difussivity of the specie in the reference gas see 22_Hlavaty_MSc

    // -- prepare free difusion coeficients of the current specie against all other species 
    // -- NOTE: at the moment we use only diffusion of the specie in the reference gas (nitrogen) -- last member of the list
    PtrList<dimensionedScalar> DABLst(specieNames.size()+1);
    forAll(DABLst,lstII)
    {
	    // diffusion coefficients according to Fuller without temperature and pressure dependence
	    DABLst.set
	    (
            lstII,
            new dimensionedScalar(
                "DAB",
                Foam::pow(dimLength,2)/dimTime/Foam::pow(dimTemperature,1.75)*dimPressure,
                ((0.014)/(Foam::sqrt(2.0/(1e-3/molMList[lstII] + 1e-3/molMList[lstI]))*Foam::pow(Foam::pow(volDiffLst[lstI],1.0/3.0)+Foam::pow(volDiffLst[lstII],1.0/3.0),2))).value()
            )
	    );
    }

    // -- Diffusion coeficient of the mixture -- default reference gas diffusion + add temperature and pressure dependence (see Fuller)
    volScalarField DifMix
    (
        DABLst[DABLst.size()-1]*Foam::pow(thermo.T(),1.75)/p
    );

    // -- prepare Knudsen diffusivity
    volScalarField DKn
    (
        IOobject
        (
            "DKn",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("DKn",DifMix.dimensions(),0.0)
    );

    DKn += freeChannel*DifMix;         // free-channel (not used, but cant be 0)
    forAll(zoneLst,countI)
    {
        DKn += zoneLst[countI]*(1.0/3.0*sqrt((thermo.T()*8*univR)/(3.141592653*molMList[lstI])))*dp[countI];
    }

    // -- prepare the effective diffusion coefficient
    volScalarField DEff
    (
        IOobject
        (
            "DEff",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedScalar("DEff",DifMix.dimensions(),0.0)
        // (1.0-max(isOnWallCoat,isInWallCoat))*DifMix +
        // isOnWallCoat*(porEpsC/(tortC*(1.0/DifMix+1.0/DKn)))+
        // isInWallCoat*(porEpsS/(tortS*(1.0/DifMix+1.0/DKn))) 
    );
    DEff += freeChannel*DifMix;         // free-channel
    forAll(zoneLst,countI)
    {
        DEff += zoneLst[countI]*(eps[countI]/(tort[countI]*(1.0/DifMix+1.0/DKn)));  // porous media
    } 

    DEff.correctBoundaryConditions();

    // --  calculation of total concentration
    volScalarField cTot(thermo.rho()/molMList[molMList.size()-1]);
    // volScalarField cTot(rho/molMList[molMList.size()-1]);

    // -- inform about calculated properties
    Info << "Min cTot: " << min(cTot).value() << ", max cTot: " << max(cTot).value() << endl;
    Info << "Min DEff[" << specieNames[lstI] << "]: " << min(DEff).value() << ", max DEff["<<specieNames[lstI]<<"]: "<< max(DEff).value() << endl;
    Info << "Min y[" << specieNames[lstI] << "]: " << min(currSp).value() << ", max y[" << specieNames[lstI] << "]: " << max(currSp).value() << endl;


    // -- calculate convective molar flux at the face 
    // surfaceScalarField phiUCTot
    // (
    //     IOobject
    //     (
    //         "phiU",
    //         runTime.timeName(),
    //         mesh,
    //         IOobject::NO_READ,
    //         IOobject::NO_WRITE
    //     ),
    //     linearInterpolate(U*cTot) & mesh.Sf()
    // );
    

    // -- balance of the specie
    fvScalarMatrix concEq
    (
        // fvm::ddt(rho,currSp)/molMList[molMList.size()-1]                                               
        // + fvm::div(phiUCTot, currSp)
        fvm::div(phi/molMList[molMList.size()-1], currSp)
        ==
        fvm::laplacian(DEff*cTot, currSp)                              
        + fvc::laplacian(DEff*currSp, cTot)       
        // + fvModels.source(dummyConcEqn, currSp)                                      // source term defined in fvOptions, linearized -- part implicit, part explicit
        + fvModels.source(dummyConcEqn, currSp)                                      // source term defined in fvModels, linearized -- part implicit, part explicit
    );
	    
    // fvOptions.constrain(concEq);
    fvConstraints.constrain(concEq);

    concEq.relax();

    concEq.solve();

    fvConstraints.constrain(currSp);

    currSp.correctBoundaryConditions();
    
    forAll(currSp,cellI)
    { 
    //	 currSp[cellI] = min(max(currSp[cellI],0),1);                    //clip unreal molar fractions
        currSp[cellI] = max(currSp[cellI],0); 
    }
}