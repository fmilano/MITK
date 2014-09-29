/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/


#ifndef MITKITKFILEIO_H
#define MITKITKFILEIO_H

#include "mitkAbstractFileIO.h"

#include <itkImageIOBase.h>

namespace mitk {

// This class wraps ITK image IO objects registered via the
// ITK object factory system
class ItkImageIO : public AbstractFileIO
{

public:

  ItkImageIO(itk::ImageIOBase::Pointer imageIO);

  // -------------- AbstractFileReader -------------

  using AbstractFileReader::Read;
  virtual std::vector<itk::SmartPointer<BaseData> > Read();

  virtual ReaderConfidenceLevel GetReaderConfidenceLevel() const;

  // -------------- AbstractFileWriter -------------

  virtual void Write();
  virtual WriterConfidenceLevel GetWriterConfidenceLevel() const;

private:

  ItkImageIO(const ItkImageIO& other);

  ItkImageIO* Clone() const;

  std::vector<std::string> FixUpImageIOExtensions(const std::string& imageIOName);

  itk::ImageIOBase::Pointer m_ImageIO;
};

} // namespace mitk

#endif /* MITKITKFILEIO_H */
