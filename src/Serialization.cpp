
#include "Serialization.h"


void BaseData<SaveDataLHS, SaveDataRHS>::SetData(SaveDataLHS formId, SaveDataRHS value) {
    Locker locker(m_Lock);
    m_Data[formId] = value;
}


void BaseData<SaveDataLHS, SaveDataRHS>::Clear() {
    Locker locker(m_Lock);
    m_Data.clear();
}

[[nodiscard]] bool SaveLoadData::Save(SKSE::SerializationInterface* serializationInterface) {
    assert(serializationInterface);
    Locker locker(m_Lock);

    const auto numRecords = m_Data.size();
    if (!serializationInterface->WriteRecordData(numRecords)) {
        logger::error("Failed to save {} data records", numRecords);
        return false;
    }

    for (const auto& [lhs, rhs] : m_Data) {
        std::uint32_t formid = lhs.first;
        if (!serializationInterface->WriteRecordData(formid)) {
            logger::error("Failed to save formid");
            return false;
        }

        const std::string editorid = lhs.second;
        logger::trace("Formid:{:x},Editorid:{}", formid, editorid);
        Utils::write_string(serializationInterface, editorid);

        if (!serializationInterface->WriteRecordData(rhs)) {
            logger::error("Failed to save hotkey");
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool SaveLoadData::Save(SKSE::SerializationInterface* serializationInterface, std::uint32_t type,
                        std::uint32_t version) {
    if (!serializationInterface->OpenRecord(type, version)) {
        logger::error("Failed to open record for Data Serialization!");
        return false;
    }

    return Save(serializationInterface);
}

[[nodiscard]] bool SaveLoadData::Load(SKSE::SerializationInterface* serializationInterface, unsigned int pluginversion) {
    assert(serializationInterface);

    if (pluginversion < 1) {
		logger::error("Plugin version is less than 0.1, skipping load.");
		return false;
	}

    std::size_t recordDataSize;
    serializationInterface->ReadRecordData(recordDataSize);
    logger::info("Loading data from serialization interface with size: {}", recordDataSize);

    Locker locker(m_Lock);
    m_Data.clear();

    logger::trace("Loading data from serialization interface.");
    for (auto i = 0; i < recordDataSize; i++) {
        std::uint32_t formid = 0;
        logger::trace("ReadRecordData:{}", serializationInterface->ReadRecordData(formid));
        if (!serializationInterface->ResolveFormID(formid, formid)) {
            logger::error("Failed to resolve form ID, 0x{:X}.", formid);
            continue;
        }

        std::string editorid;
        if (!Utils::read_string(serializationInterface, editorid)) {
            logger::error("Failed to read editorid");
            return false;
        }

        SaveDataLHS lhs({formid, editorid});
        logger::trace("Reading value...");

        if (pluginversion < 2) {
            m_Data[lhs] = -1;
        } 
        else {
			SaveDataRHS rhs;
            if (!serializationInterface->ReadRecordData(rhs)) {
				logger::error("Failed to read hotkey");
				return false;
			}
			m_Data[lhs] = rhs;
        }

        logger::info("Loaded data for formid {}, editorid {}", formid, editorid);
    }

    return true;
}

