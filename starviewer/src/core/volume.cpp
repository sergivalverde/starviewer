/***************************************************************************
 *   Copyright (C) 2005 by Grup de Gràfics de Girona                       *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#ifndef UDGVOLUME_CPP
#define UDGVOLUME_CPP

// VTK
#include <vtkImageData.h>
#include <vtkExtractVOI.h>
#include <vtkImageChangeInformation.h>
#include <vtkDICOMImageReader.h>

// ITK
#include <itkMetaDataDictionary.h>
#include <itkMetaDataObject.h>
#include <itkExtractImageFilter.h>
#include <itkTileImageFilter.h>

#include "volume.h"
#include "volumesourceinformation.h"
#include "logging.h"
#include "image.h"
#include "series.h"
#include "study.h"
#include "patient.h"

#include "input.h"

//\TODO trobar perquè això és necessari amb les dcmtk
#define HAVE_CONFIG_H 1
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmimgle/dcmimage.h"

namespace udg {

Volume::Volume()
{
    init();
}

Volume::Volume( ItkImageTypePointer itkImage )
{
    init();
    this->setData( itkImage );
}

Volume::Volume( VtkImageTypePointer vtkImage )
{
    init();
    this->setData( vtkImage );
}

void Volume::init()
{
    // \TODO és millor crear un objecte o assignar-li NUL a l'inicialitzar? Així potser és més segur des del punt de vista de si li demanem propietats al volum com origen, espaiat, etc
    m_imageDataVTK = vtkImageData::New();

    m_itkToVtkFilter = ItkToVtkFilterType::New();
    m_vtkToItkFilter = VtkToItkFilterType::New();

    m_volumeInformation = new VolumeSourceInformation;

    m_dataLoaded = false;
}

Volume::~Volume()
{
}

Volume::ItkImageTypePointer Volume::getItkData()
{
    m_vtkToItkFilter->SetInput( m_imageDataVTK );
    try
    {
        m_vtkToItkFilter->GetImporter()->Update();
    }
    catch( itk::ExceptionObject & excep )
    {
        WARN_LOG( QString("Excepció en el filtre vtkToItk :: Volume::getItkData() -> ") + excep.GetDescription() );
    }
    return m_vtkToItkFilter->GetImporter()->GetOutput();
}

Volume::VtkImageTypePointer Volume::getVtkData()
{
    if( !m_dataLoaded )
    {
        this->loadWithPreAllocateAndInsert();
    }
    return m_imageDataVTK;
}

void Volume::setData( ItkImageTypePointer itkImage  )
{
    m_itkToVtkFilter->SetInput( itkImage );
    try
    {
        m_itkToVtkFilter->Update();
    }
    catch( itk::ExceptionObject & excep )
    {
        WARN_LOG( QString("Excepció en el filtre itkToVtk :: Volume::setData( ItkImageTypePointer itkImage ) -> ") + excep.GetDescription() );
    }
    this->setData( m_itkToVtkFilter->GetOutput() );
}

void Volume::setData( VtkImageTypePointer vtkImage )
{
    // \TODO fer còpia local, no només punter-> com fer-ho?
    if( m_imageDataVTK )
        m_imageDataVTK->ReleaseData();
    m_imageDataVTK = vtkImage;
    m_dataLoaded = true;
}

void Volume::updateInformation()
{
    getVtkData()->UpdateInformation();
}

void Volume::getOrigin( double xyz[3] )
{
    getVtkData()->GetOrigin( xyz );
}

double *Volume::getOrigin()
{
    return getVtkData()->GetOrigin();
}

void Volume::getSpacing( double xyz[3] )
{
    getVtkData()->GetSpacing( xyz );
}

double *Volume::getSpacing()
{
    return getVtkData()->GetSpacing();
}

void Volume::getWholeExtent( int extent[6] )
{
    getVtkData()->GetWholeExtent( extent );
}

int *Volume::getWholeExtent()
{
    return getVtkData()->GetWholeExtent();
}

int *Volume::getDimensions()
{
    return getVtkData()->GetDimensions();
}

void Volume::getDimensions( int dims[3] )
{
    getVtkData()->GetDimensions( dims );
}

Volume *Volume::getSubVolume( int index  )
{
    int slices = this->getVolumeSourceInformation()->getNumberOfSlices();
    int *size = this->getWholeExtent();

    vtkExtractVOI * extractedVolume = vtkExtractVOI::New();
    vtkImageChangeInformation * vtkChange = vtkImageChangeInformation::New();

    extractedVolume->SetInput( m_imageDataVTK );
    extractedVolume->SetVOI( size[0] , size[1] , size[2] , size[3] , ( index * slices ) ,  ( ( index * slices ) + slices - 1 ) );

    vtkChange->SetInput( extractedVolume->GetOutput() );
    vtkChange->SetOutputOrigin( 0.0 , 0.0 , 0.0 );
    vtkChange->SetOutputExtentStart( 0 , 0 , 0 );

    Volume *subVolume = new Volume( vtkChange->GetOutput() );
    subVolume->setVolumeSourceInformation( m_volumeInformation );
    subVolume->getVtkData()->Update();

    return subVolume;
}

Volume * Volume::orderSlices()
{
    int phases, slices;
    Volume * orderedVolume;

    int *dimensions = this->getDimensions();
    ItkImageType::SizeType size;
    size[0] = dimensions[0];
    size[1] = dimensions[1];
    size[2] = 0; // Volem una imatge 2D

    ItkImageType::IndexType index;
    index[0] = 0;
    index[1] = 0;
    index[2] = 0;

    phases = this->getVolumeSourceInformation()->getNumberOfPhases();
    slices = this->getVolumeSourceInformation()->getNumberOfSlices();

    typedef ItkImageType ItkImageType3D;
    typedef itk::Image<ItkPixelType, 2 > ItkImageType2D;

    typedef itk::ExtractImageFilter< ItkImageType3D , ItkImageType2D > ExtractImageType;
    typedef itk::TileImageFilter< ItkImageType2D , ItkImageType3D  > TileFilterType;

    ExtractImageType::Pointer extractFilter = ExtractImageType::New();
    extractFilter->SetInput( this->getItkData() );

    TileFilterType::Pointer tileFilter = TileFilterType::New();

    // El layout ens serveix per indicar cap on creix la cua. En aquest cas volem fer creixer la coordenada Z
    TileFilterType::LayoutArrayType layout;
    layout[0] = 1;
    layout[1] = 1;
    layout[2] = 0;

    tileFilter->SetLayout( layout );

    ItkImageType::RegionType region;
    region.SetSize( size );

    std::vector< ItkImageType2D::Pointer > extracts;

    for ( int i = 0 ; i < phases ; i++ )
    {
        for (int j = 0 ; j < slices ; j++ )
        {
            index[2] = ( ( phases * j ) + i ) ;

            region.SetIndex( index );

            extractFilter->SetExtractionRegion( region );

            extractFilter->Update();

            extracts.push_back( extractFilter->GetOutput() );
            extracts.back()->DisconnectPipeline(); //S'ha de desconnectar la pipeline per forçar a l'extracFilter a generar un nou output.

            tileFilter->PushBackInput( extracts.back() );

        }
    }

    tileFilter->Update();

    orderedVolume = new Volume( tileFilter->GetOutput() );
    orderedVolume->setVolumeSourceInformation( m_volumeInformation );
    //orderedVolume->getVtkData()->Update();

    return orderedVolume;
}

void Volume::setImageOrderCriteria( unsigned int orderCriteria )
{
    m_imageOrderCriteria = orderCriteria;
}

unsigned int Volume::getImageOrderCriteria() const
{
    return m_imageOrderCriteria;
}

void Volume::addImage( Image *image )
{
    //\TODO tenir en compte el criteri d'ordenació aquí?
    if( !m_imageSet.contains(image) )
        m_imageSet << image;
}

void Volume::setImages( const QList<Image *> &imageList )
{
    m_imageSet.clear();
    m_imageSet = imageList;
    this->fillVolumeSourceInformationFromImages();
    m_dataLoaded = false;
}

Series *Volume::getSeries()
{
    if( !m_imageSet.isEmpty() )
    {
        return m_imageSet.at(0)->getParentSeries();
    }
    else
        return NULL;
}

void Volume::allocateImageData()
{
    //\TODO si les dades estan allotjades per defecte, fer un delete primer i després fer un new? o amb un ReleaseData n'hi ha prou?
    m_imageDataVTK->Delete();
    m_imageDataVTK = vtkImageData::New();

    // Inicialitzem les dades
    double origin[3];
    origin[0] = m_imageSet.at(0)->getImagePosition()[0];
    origin[1] = m_imageSet.at(0)->getImagePosition()[1];
    origin[2] = m_imageSet.at(0)->getImagePosition()[2];
    m_imageDataVTK->SetOrigin( origin );
    double spacing[3];
    spacing[0] = m_imageSet.at(0)->getPixelSpacing()[0];
    spacing[1] = m_imageSet.at(0)->getPixelSpacing()[1];
    spacing[2] = m_imageSet.at(0)->getSliceThickness();
    m_imageDataVTK->SetSpacing( spacing );
    m_imageDataVTK->SetDimensions( m_imageSet.at(0)->getRows(), m_imageSet.at(0)->getColumns(), m_imageSet.size() );
    //\TODO de moment assumim que sempre seran ints i ho mapejem així,potser més endavant podria canviar, però és el tipus que tenim fixat desde les itk
    m_imageDataVTK->SetScalarTypeToUnsignedShort();
    m_imageDataVTK->SetNumberOfScalarComponents(1);
    m_imageDataVTK->AllocateScalars();
}

void Volume::loadWithAppends()
{
}

void Volume::loadWithPreAllocateAndInsert()
{
    if( !m_imageSet.isEmpty() )
    {
        this->allocateImageData();
//         this->loadSliceWithDcmtk();
        this->loadSliceWithDcmtk2();
//         this->loadSliceWithVtkDICOMReader();
//         this->loadSliceWithInput();
        m_imageDataVTK->Update();
        m_dataLoaded = true;
    }
    else
    {
        DEBUG_LOG("No tenim cap imatge per carregar");
    }
}

void Volume::loadSliceWithDcmtk()
{
    int dimension[2] = {m_imageDataVTK->GetDimensions()[0] , m_imageDataVTK->GetDimensions()[1]};
    int zSlice = 0;
    // calculem quants bytes ocupen cada pixel \TODO si ja sempre allotjem com a ints? ja sabem que sempre seran dos bytes?
    int byteDepth = m_imageSet.at(0)->getBitsAllocated() / 8;
    // el nombre de bytes per llesca
    int sliceByteCount = dimension[0] * dimension[1] * byteDepth;
    // objectes dcmtk per la lectura
    DcmFileFormat dicomFile;
    DcmDataset *dicomData = NULL;

    // Llegir les imatges
    foreach( Image *image, m_imageSet )
    {
        OFCondition status = dicomFile.loadFile( qPrintable( image->getPath() ) );
        if( status.good() )
        {
            dicomData = dicomFile.getDataset();
            const Uint8 *pixelData8 = NULL;
            const Uint16 *pixelData16 = NULL;
//             status = dicomData->findAndGetUint8Array(DCM_PixelData, pixelData8);
//             if( status.good() )
//             {
//                 sliceByteCount = m_imageDataVTK->GetDimensions()[0] * m_imageDataVTK->GetDimensions()[1] * sizeof(Uint8);
//                 // copiem les dades del buffer a l'image data
//                 memcpy( (Uint8 *)m_imageDataVTK->GetScalarPointerForExtent( sliceExtent ), pixelData8, sliceByteCount );
//
//                 status = dicomData->findAndGetUint16Array(DCM_PixelData, pixelData16);
//                 if( status.good() ){
//                     DEBUG_LOG("Llegir 16 bits tampoc falla :D");}
//                 else
//                     DEBUG_LOG( QString( "Error en llegir les dades dels pixels 16 bits. Error: %1 ").arg( status.text() ) );
//             }
//             else
//             {
                status = dicomData->findAndGetUint16Array(DCM_PixelData, pixelData16);
                if( status.good() )
                {
//                     sliceByteCount = m_imageDataVTK->GetDimensions()[0] * m_imageDataVTK->GetDimensions()[1] * sizeof(Uint16);
                    // copiem les dades del buffer a l'image data
                    memcpy( (Uint16 *)m_imageDataVTK->GetScalarPointer(0,0,zSlice), pixelData16, sliceByteCount );
                }
                else
                    DEBUG_LOG( QString( "Error en llegir les dades dels pixels. Error: %1 ").arg( status.text() ) );
//             }
        }
        else
        {
            DEBUG_LOG( QString( "Error en llegir l'arxiu [%1]\n%2 ").arg( image->getPath() ).arg( status.text() ) );
        }
        // avancem el punter perquè escrigui sobre la següent posició de memòria
        zSlice++;
    }
}

void Volume::loadSliceWithDcmtk2()
{
    int zSlice = 0;
//     unsigned long imageSizeInBytes = 0;
    // buffer on colocarem la imatge dcmtk
//     void *dcmtkImageBuffer = NULL;
    // Llegir les imatges
    foreach( Image *image, m_imageSet )
    {
        DEBUG_LOG( QString("Tractant llesca: %1 ").arg(zSlice) );
        DicomImage *dicomImage = new DicomImage( qPrintable( image->getPath() ) );
        if( dicomImage != NULL )
        {
            if( dicomImage->getStatus() == EIS_Normal )
            {
                dicomImage->setMinMaxWindow();
//                 dicomImage->writePPM( qPrintable(QString("/home/chus/prova%1.ppm").arg(zSlice) ) );
//                 dcmtkImageBuffer = (void *)dicomImage->getOutputData(16);
//                 if( dcmtkImageBuffer != NULL )
                if( dicomImage->getOutputData(16) != NULL )
                {
//                     imageSizeInBytes = dicomImage->getOutputDataSize();
//                     DEBUG_LOG( QString("Copiant imatge de %1 bytes").arg( imageSizeInBytes ) );
//                     memcpy((unsigned short *)m_imageDataVTK->GetScalarPointer(0,0,zSlice), (unsigned short *)dcmtkImageBuffer, imageSizeInBytes );
                    memcpy(m_imageDataVTK->GetScalarPointer(0,0,zSlice), dicomImage->getOutputData(16), dicomImage->getOutputDataSize() );
                    dicomImage->deleteOutputData();
                    m_imageDataVTK->Modified();
                }
                else
                    DEBUG_LOG( QString( "No hem pogut obtenir les dades dels pixels del DicomImage. Error: %1 ").arg( DicomImage::getString( dicomImage->getStatus() ) ) );
            }
            else
                DEBUG_LOG( QString( "Error en carregar la DicomImage. Error: %1 ").arg( DicomImage::getString( dicomImage->getStatus() ) ) );
        }
        zSlice++;
    }
}

void Volume::loadSliceWithVtkDICOMReader()
{
    int zSlice = 0;
    // obtenim el punter inicial de les dades d'imatge
    unsigned short *scalarBuffer = (unsigned short *)m_imageDataVTK->GetScalarPointer(0,0,zSlice);
    vtkDICOMImageReader *reader = vtkDICOMImageReader::New();
    // buffer on colocarem la llesca
    unsigned short *imageBuffer = NULL;
    // Llegir les imatges
    foreach( Image *image, m_imageSet )
    {
        DEBUG_LOG( QString("Llesca que vull carregar: %1").arg( zSlice ) );
        reader->SetFileName( qPrintable( image->getPath() ) );
        reader->Update();
        imageBuffer = (unsigned short *)reader->GetOutput()->GetScalarPointer();
        scalarBuffer = (unsigned short *)m_imageDataVTK->GetScalarPointer(0,0,zSlice);
        // copiem les dades del buffer a l'image data
        memcpy( scalarBuffer, imageBuffer, m_imageDataVTK->GetDimensions()[0]*m_imageDataVTK->GetDimensions()[1]*2 );
        DEBUG_LOG( "Valor d'un pixel del mig(scalarBuffer): " + QString("%1").arg(scalarBuffer[256*256+256]) );
        zSlice++;
    }
}

void Volume::loadSliceWithInput()
{
    int dimension[2] = {m_imageDataVTK->GetDimensions()[0] , m_imageDataVTK->GetDimensions()[1]};
    int zSlice = 0;
    // calculem quants bytes ocupen cada pixel \TODO si ja sempre allotjem com a ints? ja sabem que sempre seran dos bytes?
    int byteDepth = m_imageSet.at(0)->getBitsAllocated() / 8;
    // el nombre de bytes per llesca
    int sliceByteCount = dimension[0] * dimension[1] * byteDepth;

    // obtenim el punter inicial de les dades d'imatge
    int *scalarBuffer = (int *)m_imageDataVTK->GetScalarPointer();

    // buffer on colocarem la llesca
    int *imageBuffer = NULL;

    int sliceExtent[6] = {0,dimension[0]-1, 0, dimension[1]-1, zSlice, zSlice};
    // Llegir les imatges
    foreach( Image *image, m_imageSet )
    {
        DEBUG_LOG( QString("Llesca que vull carregar: %1").arg( zSlice ) );
        Input *input = new Input;
        input->openFile( image->getPath() );
        imageBuffer = (int *)input->getData()->getVtkData()->GetScalarPointer();
        // copiem les dades del buffer a l'image data
        memcpy( scalarBuffer, imageBuffer, sliceByteCount );
        DEBUG_LOG( "Valor d'un pixel del mig(scalarBuffer): " + QString("%1").arg(scalarBuffer[256*256+256]) );
        // avancem el punter perquè escrigui sobre la següent posició de memòria
        scalarBuffer += sliceByteCount;
        m_imageDataVTK->Update();
        zSlice++;
//         sliceExtent[4] = sliceExtent[5] = zSlice;
//         scalarBuffer = (int *)m_imageDataVTK->GetScalarPointerForExtent( sliceExtent );
    }
}

void Volume::fillVolumeSourceInformationFromImages()
{
    Image *firstImage = m_imageSet.at(0);
    Series *series = firstImage->getParentSeries();

    m_volumeInformation->setPatientPosition( series->getPatientPosition() );
    m_volumeInformation->setPatientOrientationString( firstImage->getPatientOrientation() );
    m_volumeInformation->setDirectionCosines( (double *)firstImage->getImageOrientation() );
    m_volumeInformation->setInstitutionName( series->getInstitutionName() );
    m_volumeInformation->setPatientName( series->getParentStudy()->getParentPatient()->getFullName() );
    m_volumeInformation->setPatientID( series->getParentStudy()->getParentPatient()->getID() );
    m_volumeInformation->setStudyDate( series->getParentStudy()->getDateAsString() );
    m_volumeInformation->setStudyTime( series->getParentStudy()->getTimeAsString() );
    m_volumeInformation->setAccessionNumber( series->getParentStudy()->getAccessionNumber() );
    m_volumeInformation->setProtocolName( series->getProtocolName() );
    // aquests encara no els tenim
    m_volumeInformation->setNumberOfPhases( 1 );
//     m_volumeInformation->setNumberOfSlices( ?? );
}

};

#endif
