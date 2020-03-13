/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
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

#ifndef SFNSF_H
#define SFNSF_H

#include <ns3/simple-ref-count.h>

namespace ns3 {

class SfnSf : public SimpleRefCount<SfnSf>
{
public:
  SfnSf () = default;

  SfnSf (uint16_t frameNum, uint8_t sfNum, uint16_t slotNum, uint8_t varTtiNum);

  uint64_t Encode () const;

  /**
   * \return the number of subframes per frame, 10
   */
  static uint32_t GetSubframesPerFrame ();

  static uint64_t Encode (const SfnSf &p);

  void Decode (uint64_t sfn);

  static SfnSf FromEncoding (uint64_t sfn);

  SfnSf IncreaseNoOfSlots (uint32_t slotsPerSubframe) const;

  SfnSf CalculateUplinkSlot (uint32_t k2Delay, uint32_t slotsPerSubframe) const;

  SfnSf IncreaseNoOfSlotsWithLatency (uint32_t latency, uint32_t slotsPerSubframe) const;

  /**
   * \brief Normalize the SfnSf in slot number
   * \param slotsPerSubframe Number of slot per subframe
   * \param subframesPerFrame Number of subframes per frame
   * \return The number of total slots passed (can overlap)
   */
  uint64_t Normalize (uint32_t slotsPerSubframe) const;

  /**
   * \brief Add to this SfnSf a number of slot indicated by the first parameter
   * \param slotN Number of slot to add
   * \param slotsPerSubframe Number of slot per subframe
   * \param subframesPerFrame Number of subframes per frame
   */
  void
  Add (uint32_t slotN, uint32_t slotsPerSubframe);

  /**
   * \brief operator < (less than)
   * \param rhs other SfnSf to compare
   * \return true if this SfnSf is less than rhs
   *
   * The comparison is done on m_frameNum, m_subframeNum, and m_slotNum: not on varTti
   */
  bool operator < (const SfnSf& rhs) const;

  /**
   * \brief operator ==, compares only frame, subframe, and slot
   * \param o other instance to compare
   * \return true if this instance and the other have the same frame, subframe, slot
   *
   * Used in the MAC operation, in which the varTti is not used (or, it is
   * its duty to fill it).
   *
   * To check the varTti, please use this operator and IsTtiEqual()
   */
  bool operator == (const SfnSf &o) const;

  /**
   * \brief Compares frame, subframe, slot, and varTti
   * \param o other instance to compare
   * \return true if this instance and the other have the same frame, subframe, slot, and varTti
   *
   * Used in PHY or in situation where VarTti is needed.
   */
  bool IsTtiEqual (const SfnSf &o) const;

  uint16_t m_frameNum   {0}; //!< Frame Number
  uint8_t m_subframeNum {0}; //!< SubFrame Number
  uint16_t m_slotNum    {0}; //!< Slot number (a slot is made by 14 symbols)
  uint8_t m_varTtiNum   {0}; //!< Equivalent to symStart: it is the symbol in which the sfnsf starts
};

} // namespace ns3
#endif // SFNSF_H
