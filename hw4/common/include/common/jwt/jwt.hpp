#pragma once

#include <stdexcept>
#include <string>
#include <chrono>
#include <optional>

#include <jwt-cpp/jwt.h>
#include <userver/formats/json.hpp>
#include <userver/http/common_headers.hpp>
#include <jwt-cpp/traits/boost-json/defaults.h>

namespace common {

inline const std::string kParticipantRole = "participant";
inline const std::string kOrganizerRole = "organizer";
inline const std::string kAdminRole = "admin";

struct JwtPayload {
    std::string user_id;
    std::string role;
};

class JwtManager {
public:
    explicit JwtManager(std::string secret_key)
        : secret_(std::move(secret_key)) {}

    std::string GenerateToken(const std::string &user_id,
                              const std::string &role,
                              int duration_hours = 24 * 7) const {
      if (role != kParticipantRole && role != kOrganizerRole && role != kAdminRole) {
        throw std::invalid_argument("invalid role");
      }

      auto now = std::chrono::system_clock::now();
      auto expires_at = now + std::chrono::hours{duration_hours};

      return jwt::create()
          .set_issuer("EventService")
          .set_type("JWT")
          .set_issued_at(now)
          .set_expires_at(expires_at)
          .set_payload_claim("user_id", jwt::claim(user_id))
          .set_payload_claim("role", jwt::claim(role))
          .sign(jwt::algorithm::hs256{secret_});
    }

    std::optional<JwtPayload> VerifyToken(const std::string& token) const {
        if (token.empty()) {
            return std::nullopt;
        }

        try {
            auto decoded = jwt::decode(token);

            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{secret_})
                .with_issuer("EventService");

            verifier.verify(decoded);

            JwtPayload payload;
            payload.user_id = decoded.get_payload_claim("user_id").as_string();
            payload.role = decoded.get_payload_claim("role").as_string();

            if (payload.role != kParticipantRole && payload.role != kOrganizerRole &&
                payload.role != kAdminRole) {
              throw std::invalid_argument("invalid role");
            }

            return payload;

        } catch (const std::exception& /*e*/) {
            return std::nullopt;
        }
    }

private:
    std::string secret_;
};

} // namespace common
