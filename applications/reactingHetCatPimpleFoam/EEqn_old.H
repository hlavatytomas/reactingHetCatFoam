{
    // -- load temperature from thermo package
    // T = thermo.T();
  // thermo.correct();

    volScalarField& T = thermo.T();
    // volScalarField& he = thermo.he();
    
    // -- calculate the effective thermal diffusivity
    volScalarField kappa (kappaA+kappaB*T+kappaC*Foam::pow(T,2)+kappaD*Foam::pow(T,3));
    volScalarField kappaEff
    (
      IOobject
      (
          "kappaEff",
          runTime.timeName(),
          mesh,
          IOobject::NO_READ,
          IOobject::NO_WRITE
      ),
      mesh,
      dimensionedScalar("kappaEff",kappa.dimensions(),0.0)
    );

    // -- efective kappa values, gas kappa in free channel, media kappa in zones
    kappaEff += freeChannel*kappa;         
    forAll(zoneLst,countI)
    {
      kappaEff += zoneLst[countI]*(kappaZ[countI]);
    } 
    
    // -- calculate heat capacity of the gas (reference at the moment)
    // NOTE: old, now using OpenFOAM thermo
    // volScalarField Cp
    // (
    //   (ALst[ALst.size()-1] + BLst[BLst.size()-1] * T + CLst[CLst.size()-1] * Foam::pow(T,2) + DLst[DLst.size()-1] * Foam::pow(T,3)) / molMList[molMList.size()-1]
    // );
      	
    // NOTE: discuss how to treat CpEff and rhoEff (probably will be important for dynamics)
    // -- calculate efective heat capacity 
    volScalarField CpEff
    (
      IOobject
      (
          "CpEff",
          runTime.timeName(),
          mesh,
          IOobject::NO_READ,
          IOobject::NO_WRITE
      ),
      mesh,
      dimensionedScalar("CpEff",thermo.Cp().dimensions(),0.0)
    );

    // -- efective Cp values, gas kappa in free channel, media kappa in zones
    CpEff += freeChannel*thermo.Cp();         
    forAll(zoneLst,countI)
    {
      CpEff += zoneLst[countI]*(CpZ[countI]);
    } 
    
    //efective rho
    volScalarField rhoEff
    (
      IOobject
      (
          "rhoEff",
          runTime.timeName(),
          mesh,
          IOobject::NO_READ,
          IOobject::NO_WRITE
      ),
      mesh,
      dimensionedScalar("rhoEff",rho.dimensions(),0.0)
    );

    rhoEff += freeChannel*thermo.rho();     
    forAll(zoneLst,countI)
    {
      rhoEff += zoneLst[countI]*(rhoZ[countI]);
    } 

    rhoEff.correctBoundaryConditions();
    kappaEff.correctBoundaryConditions();
    CpEff.correctBoundaryConditions();
    
    // dissipation rate  -- not used at the moment (neglectable)
    // volScalarField K (0.5*magSqr(U));
      
    //efective rho * Cp
    // volScalarField rhoCpEff(thermo.rho()*thermo.Cp());

    // -- minimal and maximal values of calculated fields
    Info << "Min (kappaEff): " << min(kappaEff).value() << ", max (kappaEff): " << max(kappaEff).value() << "." << endl;
    Info << "Min (thermo.Cp): " << min(thermo.Cp()).value() << ", max (Cp): " << max(thermo.Cp()).value() << "." << endl;
    Info << "Min (thermo.rho): " << min(thermo.rho()).value() << ", max (thermo.rho): " << max(thermo.rho()).value() << "." << endl;
    Info << "Min rhoEff: " << min(rhoEff).value() << ", max rhoEff: " << max(rhoEff).value() << "." << endl;
    Info << "Min CpEff: " << min(CpEff).value() << ", max CpEff: " << max(CpEff).value() << "." << endl;
    
    // enthalpy balance
    fvScalarMatrix EEqn
    (
        // CpEff*fvm::ddt(rhoEff, T) +
        // thermo.Cp()*fvm::ddt(thermo.rho(), T) +
        CpEff*rhoEff*fvm::ddt(T) + fvm::Sp(fvc::ddt(CpEff,rhoEff),T)+
        // CpEff*fvm::ddt(rhoEff, T) +
        fvc::ddt(rho, K) + fvc::div(phi, K) +
        fvm::div(linearInterpolate(thermo.Cp())*phi, T)
        -dpdt
        ==
        fvm::laplacian(kappaEff, T)
        + fvModels.source(dummyEEqn, T)

      //   fvm::ddt(rho, he) 
      //   + fvm::div(phi, he)
      //   // -dpdt
      // ==
      //   fvm::laplacian(kappaEff/thermo.Cp(), he)
      //   + fvModels.source(rho, he)
    );
    
    EEqn.relax();

    fvConstraints.constrain(EEqn);

    EEqn.solve();

    // fvOptions.correct(thermo.T());
    fvConstraints.constrain(T);
    // fvConstraints.constrain(he);

    // thermo.he() = T * thermo.Cp();
    thermo.he() = T * CpEff;

    thermo.correct();

    Info << "Min (thermo.T): " << min(thermo.T()).value() << ", max (thermo.T): " << max(thermo.T()).value() << "." << endl;
    
    //~ radiation->correct();
}
