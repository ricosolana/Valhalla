#pragma once

class UserProfile {
public:
	// all relatively confusing verbose names

	std::string m_name;
	std::string m_gamerTag;
	std::string m_networkUserId;

	UserProfile(const std::string& name,
		const std::string& gamerTag, 
		const std::string& networkUserId)
	: m_name(name), m_gamerTag(gamerTag), m_networkUserId(networkUserId) {

	}

};
