Info<< "Reading the concentrations \n" << endl;

// prepare list to store the concentrations in
PtrList<volScalarField> MFrList(specieNames.size());

forAll(specieNames,countI)
{
    word currentSp = specieNames[countI];
    Info<< "Reading the concentration of the: " << currentSp << endl;

    MFrList.set
    (
        countI,
        new volScalarField
        (
            IOobject
            (
                currentSp,
                runTime.timeName(),
                mesh,
                IOobject::MUST_READ,
                IOobject::AUTO_WRITE
            ),
            mesh
        )
    );
}

// prepare list to store volScalarField  with the zone inside
// NOTE: 1 - zone is active here
//       0 - zone is not active here

PtrList<volScalarField> zoneLst(zoneNames.size());
forAll(zoneNames,countI)
{
    word currentZone = zoneNames[countI];
    Info<< "Reading the zone " << currentZone << " and saving it as volScalarField." << endl;

    bool loadFromFile = false;

    // -- load cellZone as 0 volScalarField
    if (transportProperties.subDict(currentZone).found("loadFromFile"))
    {
        transportProperties.subDict(currentZone).lookup("loadFromFile") >> loadFromFile;
    }

    if (loadFromFile)
    {
        zoneLst.set
        (
            countI,
            new volScalarField
            (
                IOobject
                (
                    currentZone,
                    runTime.timeName(),
                    mesh,
                    IOobject::MUST_READ,
                    IOobject::AUTO_WRITE
                ),
                mesh
                // dimensionedScalar (currentZone,dimless,0.0),
                // "zeroGradient"
            )
        );
        Info << "Zone " << currentZone << " has been loaded from file." << endl;
    }
    else
    {
        zoneLst.set
        (
            countI,
            new volScalarField
            (
                IOobject
                (
                    currentZone,
                    runTime.timeName(),
                    mesh,
                    IOobject::NO_READ,
                    IOobject::AUTO_WRITE
                ),
                mesh,
                dimensionedScalar (currentZone,dimless,0.0),
                "zeroGradient"
            )
        );
        Info << "Zone " << currentZone << " has been created from cellZone." << endl;
    }
}

// -- set to 1, where the cellZone is
forAll(zoneNames,countI)
{
    word currentZone = zoneNames[countI];

    labelList cellZoneIDs_ = mesh.cellZones().findIndices(currentZone);

    const labelList& cells = mesh.cellZones()[cellZoneIDs_[0]];
    forAll(cells, celli)
    {
        zoneLst[countI][cells[celli]] = 1;
    }
    zoneLst[countI].correctBoundaryConditions();
    
}

// -- porosity
volScalarField epsEff
(
    IOobject
        (
            "epsEff",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
    mesh,
    dimensionedScalar("epsEff",dimless,0.0)
);

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
    dimensionedScalar("DEff",dimLength*dimLength/dimTime,0.0)
    // (1.0-max(isOnWallCoat,isInWallCoat))*DifMix +
    // isOnWallCoat*(porEpsC/(tortC*(1.0/DifMix+1.0/DKn)))+
    // isInWallCoat*(porEpsS/(tortS*(1.0/DifMix+1.0/DKn))) 
);


// -- free channels --> free channels = 1 - sum all zones 
volScalarField sumZones
(
    IOobject
    (
        "sumZones",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("sumZones",dimless,0.0)
); 

forAll(zoneLst,countI)
{
    sumZones += zoneLst[countI];
    epsEff += zoneLst[countI]*(eps[countI]);
}
volScalarField freeChannel = dimensionedScalar("jedna",dimless,1.0)-sumZones;
epsEff += freeChannel;


// -- reading field U
Info<< "Reading field U\n" << endl;
volVectorField U
(
    IOobject
    (
        "U",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);

// -- dummy volScalarField to resolve dimension problem in conc.H fvOptions inconsistent source
volScalarField dummyConcEqn
(
    IOobject
    (
        "dummyConcEqn",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar ("dummyConcEqn",dimMoles/dimVolume,1.0)
    // dimensionedScalar ("dummyConcEqn",dimless,1.0)
);

// -- dummy volScalarField to resolve dimension problem in conc.H fvOptions inconsistent source
//NOTE: I dont know why this is necessary, dimensions are correct
//but openFOAM doesnt think so :(
volScalarField dummyEEqn
(
    IOobject
    (
        "dummyEEqn",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar ("dummyEEqn",dimMass/dimLength*Foam::pow(dimTime,-2)/dimTemperature,1.0)
);

// -- T field from mesh
// Info<< "Reading field T\n" << endl;
// volScalarField T
// (
//     IOobject
//     (
//         "T",
//         runTime.timeName(),
//         mesh,
//         IOobject::MUST_READ,
//         IOobject::AUTO_WRITE
//     ),
//     mesh
// );

//-- surface field for kappaG interpolation

Info<< "Reading thermophysical properties\n" << endl;

autoPtr<fluidThermo> pThermo
(
    fluidThermo::New(mesh)
);


fluidThermo& thermo = pThermo();
thermo.validate(args.executable(), "h", "e");

volScalarField& p = thermo.p();

volScalarField rho
(
    IOobject
    (
        "rho",
        runTime.timeName(),
        mesh,
        IOobject::READ_IF_PRESENT,
        IOobject::AUTO_WRITE
    ),
    thermo.rho()
    // p/univR/T*molMList[molMList.size()-1]
);


#include "compressibleCreatePhi.H"

pressureReference pressureReference
(
    p,
    simple.dict(),
    thermo.incompressible()
);

mesh.schemes().setFluxRequired(p.name());


Info<< "Creating turbulence model\n" << endl;
autoPtr<compressible::momentumTransportModel> turbulence
(
    compressible::momentumTransportModel::New
    (
        rho,
        U,
        phi,
        thermo
    )
);

//-- surface field for kappaG interpolation
surfaceScalarField MGradT
(
    IOobject
    (
        "MGradT",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    // linearInterpolate(U*rho*Cp) & mesh.Sf()
    fvc::snGrad(epsEff)
);

//-- surface fields for sources computation
surfaceScalarField gradCCO
(
    IOobject
    (
        "gradCCO",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    // dimensionedScalar ("dummyEEqn",dimMass/dimLength*Foam::pow(dimTime,-2)/dimTemperature,1.0)
    // linearInterpolate(U*rho*Cp) & mesh.Sf()
    // fvc::snGrad(MFrList[0])*mag(mesh.Sf())
    // fvc::snGrad(MFrList[0]*thermo.p()/thermo.T()/univR)
    fvc::snGrad(dimensionedScalar("jedna",dimLength*dimLength/dimTime,1)*MFrList[0]*thermo.p()/thermo.T()/univR)

);
surfaceScalarField gradYCO
(
    IOobject
    (
        "gradYCO",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    // linearInterpolate(U*rho*Cp) & mesh.Sf()
    // fvc::snGrad(MFrList[0])*mag(mesh.Sf())
    // fvc::snGrad(MFrList[0]*thermo.p()/thermo.T()/univR)
    fvc::snGrad(dimensionedScalar("jedna",dimLength*dimLength/dimTime,1)*MFrList[0])
);

//-- surface field for kappaG interpolation
surfaceScalarField cCOS
(
    IOobject
    (
        "cCOS",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    // linearInterpolate(U*rho*Cp) & mesh.Sf()
    fvc::interpolate(dimensionedScalar("jedna",dimLength*dimLength/dimTime,1)*MFrList[0]*thermo.p()/thermo.T()/univR)
);

//-- surface field for kappaG interpolation
surfaceScalarField yCOS
(
    IOobject
    (
        "yCOS",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    // linearInterpolate(U*rho*Cp) & mesh.Sf()
    fvc::interpolate(dimensionedScalar("jedna",dimLength*dimLength/dimTime,1)*MFrList[0])
);

Info<< "Creating thermophysical transport model\n" << endl;
autoPtr<fluidThermophysicalTransportModel> thermophysicalTransport
(
    fluidThermophysicalTransportModel::New(turbulence(), thermo)
);

dimensionedScalar initialMass = fvc::domainIntegrate(rho);

#include "createMRF.H"
#include "createFvModels.H"
#include "createFvConstraints.H"




