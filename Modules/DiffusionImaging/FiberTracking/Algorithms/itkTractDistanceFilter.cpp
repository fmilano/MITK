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
#include "itkTractDistanceFilter.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <vnl/vnl_sparse_matrix.h>

namespace itk{

TractDistanceFilter::TractDistanceFilter()
  : m_NumPoints(12)
  , disp(0)
{

}

TractDistanceFilter::~TractDistanceFilter()
{
  for (auto m : m_Metrics)
    delete m;
}

std::vector<int> TractDistanceFilter::GetIndices() const
{
  return m_Indices;
}

std::vector<float> TractDistanceFilter::GetDistances() const
{
  return m_Distances;
}

void TractDistanceFilter::SetTracts2(const std::vector<FiberBundle::Pointer> &Tracts2)
{
  m_Tracts2 = Tracts2;
}

void TractDistanceFilter::SetTracts1(const std::vector<FiberBundle::Pointer> &Tracts1)
{
  m_Tracts1 = Tracts1;
}

void TractDistanceFilter::SetMetrics(const std::vector<mitk::ClusteringMetric *> &Metrics)
{
  m_Metrics = Metrics;
}

std::vector<vnl_matrix<float> > TractDistanceFilter::ResampleFibers(mitk::FiberBundle::Pointer tractogram)
{
  std::streambuf *old = cout.rdbuf(); // <-- save
  std::stringstream ss;
  std::cout.rdbuf (ss.rdbuf());       // <-- redirect
  mitk::FiberBundle::Pointer temp_fib = tractogram->GetDeepCopy();
  temp_fib->ResampleToNumPoints(m_NumPoints);

  std::vector< vnl_matrix<float> > out_fib;

  for (int i=0; i<temp_fib->GetFiberPolyData()->GetNumberOfCells(); i++)
  {
    vtkCell* cell = temp_fib->GetFiberPolyData()->GetCell(i);
    int numPoints = cell->GetNumberOfPoints();
    vtkPoints* points = cell->GetPoints();

    vnl_matrix<float> streamline;
    streamline.set_size(3, m_NumPoints);
    streamline.fill(0.0);

    for (int j=0; j<numPoints; j++)
    {
      double cand[3];
      points->GetPoint(j, cand);

      vnl_vector_fixed< float, 3 > candV;
      candV[0]=cand[0]; candV[1]=cand[1]; candV[2]=cand[2];
      streamline.set_column(j, candV);
    }

    out_fib.push_back(streamline);
  }

  std::cout.rdbuf (old);              // <-- restore
  return out_fib;
}

float TractDistanceFilter::calc_distance(const std::vector<vnl_matrix<float> >& T1, const std::vector<vnl_matrix<float> >& T2)
{
  float distance = 0;
  for (auto metric : m_Metrics)
  {
    unsigned int r = 0;
    for (auto f1 : T1)
    {
      unsigned int c = 0;
      float min_d = 99999;
      for (auto f2 : T2)
      {
#pragma omp critical
        ++disp;
        bool flipped = false;
        float d = metric->CalculateDistance(f1, f2, flipped);
        if (d < min_d)
          min_d = d;
        ++c;
      }
      distance += min_d;
      ++r;
    }
  }
  distance /= (T1.size() *  m_Metrics.size());
  return distance;
}

void TractDistanceFilter::GenerateData()
{
  if (m_Metrics.empty())
  {
    mitkThrow() << "No metric selected!";
    return;
  }

  m_Indices.clear();
  m_Distances.clear();
  std::vector<std::vector<vnl_matrix<float>>> T1;
  std::vector<std::vector<vnl_matrix<float>>> T2;

  unsigned int num_fibs1 = 0;
  unsigned int num_fibs2 = 0;
  for (auto t : m_Tracts1)
  {
    T1.push_back(ResampleFibers(t));
    num_fibs1 += T1.back().size();
  }
  for (auto t : m_Tracts2)
  {
    T2.push_back(ResampleFibers(t));
    num_fibs2 += T2.back().size();
  }

  disp.restart(m_Metrics.size() * num_fibs1 * num_fibs2);

  m_Indices.resize(T1.size());
  m_Distances.resize(T1.size());
#pragma omp parallel for
  for (unsigned int i=0; i<T1.size(); ++i)
  {
    auto tracto1 = T1.at(i);
#pragma omp critical
    m_Distances[i] = 99999;
    for (unsigned int j=0; j<T2.size(); ++j)
    {
      auto tracto2 = T2.at(j);
      float d = calc_distance(tracto1, tracto2);
#pragma omp critical
      if (d < m_Distances[i])
      {
        m_Distances[i] = d;
        m_Indices[i] = j;
      }
    }
  }
}

}




