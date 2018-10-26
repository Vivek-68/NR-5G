/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License version 2 as
*   published by the Free Software Foundation;
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/

#ifndef ANTENNA_ARRAY_3GPP_MODEL_H_
#define ANTENNA_ARRAY_3GPP_MODEL_H_
#include <ns3/antenna-model.h>
#include <complex>
#include <ns3/net-device.h>
#include "antenna-array-model.h"
#include <map>

namespace ns3 {

class AntennaArray3gppModel : public AntennaArrayModel
{
public:

  AntennaArray3gppModel ();

  virtual ~AntennaArray3gppModel ();

  static TypeId GetTypeId ();

  void Set3gppParameters (bool isUe);

  virtual double GetRadiationPattern (double vAngle, double hAngle = 0) override;

private:

  double m_hpbw;  //HPBW value of each antenna element
  double m_gMax; //directivity value expressed in dBi and valid only for TRP (see table A.1.6-3 in 38.802)

};

std::ostream & operator<< (std::ostream & os, AntennaArrayModel::BeamId const & item);

} /* namespace ns3 */

#endif /* SRC_ANTENNA_3GPP_ANTENNA_MODEL_H_ */
