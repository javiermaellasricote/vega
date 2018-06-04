#include "vega/anonymizer.h"

#include "vega/dicom/file.h"
#include "vega/dicom/data_set.h"
#include "vega/dicom/data_element.h"
#include "vega/randomizer.h"
#include "vega/visitor.h"

#include <cstdlib>

namespace vega {
  Anonymizer::Anonymizer(const std::string& patient_id, std::function<bool(dicom::DataElement&)> custom_anonymizer)
    :
      randomizer_(std::make_shared<Randomizer>()),
      patient_id_(patient_id),
      custom_anonymizer_(custom_anonymizer)
  {
  }

  void Anonymizer::anonymize(dicom::File& file) const {
    anonymize(*file.data_set());
  }

  void Anonymizer::anonymize(dicom::DataSet& data_set) const {
    Visitor v {
      [this](dicom::DataElement& data_element) -> bool {
        // Remove certain elements (e.g. private elements)
        if (this->should_remove(data_element)) return true;

        // Apply default anonymization
        if (this->anonymize(data_element)) return true;

        // Apply custom anonymizer if present
        if (this->custom_anonymizer_) return this->custom_anonymizer_(data_element);
        // Else we are going to keep this data element
        else return false;
      }
    };

    v.visit(data_set);
  }

  bool Anonymizer::anonymize(dicom::DataElement& data_element) const {
    if (data_element.vr() == vr::PN) {
      auto manipulator = data_element.get_manipulator<PN_Manipulator>();
      for (auto& name : *manipulator) {
        name = randomizer_->generate<PN_Manipulator::value_type>();
      }
    }

    // These are patient specific data elements (group of 0x0010),
    // for example OtherPatientIDs and PatientBirthDate are members
    else if (data_element.tag().group() == dictionary::PatientName::tag.group()) {
      // Patient ID is handled separately
      if (data_element.tag() == dictionary::PatientID::tag) {
        auto manipulator = data_element.get_manipulator<LO_Manipulator>();
        manipulator->at(0) = patient_id_.empty() ? randomizer_->generate<std::string>() : patient_id_;
      }
      else {
        // Remove if sequence
        if (data_element.is_sequence()) return true;

        // Otherwise, blank internal data
        blank_data_element(data_element);
      }
    }

    // Remove "curve" elements (those with group 0x5000)
    else if (data_element.tag().group() == dictionary::CurveDimensions::tag_mask.value_tag().group()) {
      return true;
    }

    return false;
  }

  void Anonymizer::blank_data_element(dicom::DataElement& data_element) const {
    auto blank_manipulator = manipulator_for(data_element);
    data_element.set_manipulator(blank_manipulator);
  }

  bool Anonymizer::should_remove(const dicom::DataElement& data_element) const {
    return data_element.tag().is_private();
  }
}