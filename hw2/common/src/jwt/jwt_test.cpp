#include <common/jwt/jwt.hpp>
#include <userver/utest/utest.hpp>
#include <chrono>
#include <thread>

using common::JwtManager;

UTEST(JwtManager, GenerateToken_Success) {
    JwtManager jwt_manager("test_secret_key_12345");
    
    const std::string user_id = "user123";
    const std::string role = common::kParticipantRole;
    
    auto token = jwt_manager.GenerateToken(user_id, role, 24);
    
    ASSERT_FALSE(token.empty());
}

UTEST(JwtManager, GenerateToken_DifferentUsers) {
    JwtManager jwt_manager("test_secret_key_12345");
    
    const std::string user_id1 = "user123";
    const std::string user_id2 = "user456";
    const std::string role = common::kParticipantRole;
    
    auto token1 = jwt_manager.GenerateToken(user_id1, role, 24);
    auto token2 = jwt_manager.GenerateToken(user_id2, role, 24);
    
    ASSERT_FALSE(token1.empty());
    ASSERT_FALSE(token2.empty());
    EXPECT_NE(token1, token2); // Different users should have different tokens
}

UTEST(JwtManager, VerifyToken_Success) {
    JwtManager jwt_manager("test_secret_key_12345");
    
    const std::string user_id = "user123";
    const std::string role = common::kOrganizerRole;
    
    auto token = jwt_manager.GenerateToken(user_id, role, 24);
    auto payload = jwt_manager.VerifyToken(token);
    
    ASSERT_TRUE(payload.has_value());
    EXPECT_EQ(payload->user_id, user_id);
    EXPECT_EQ(payload->role, role);
}

UTEST(JwtManager, VerifyToken_InvalidToken) {
    JwtManager jwt_manager("test_secret_key_12345");
    
    const std::string invalid_token = "invalid.token.here";
    
    auto payload = jwt_manager.VerifyToken(invalid_token);
    
    EXPECT_FALSE(payload.has_value());
}

UTEST(JwtManager, VerifyToken_EmptyToken) {
    JwtManager jwt_manager("test_secret_key_12345");
    
    const std::string empty_token = "";
    
    auto payload = jwt_manager.VerifyToken(empty_token);
    
    EXPECT_FALSE(payload.has_value());
}

UTEST(JwtManager, VerifyToken_WrongSecret) {
    JwtManager jwt_manager("test_secret_key_12345");
    
    const std::string user_id = "user123";
    const std::string role = common::kAdminRole;
    
    // Generate token with one secret
    auto token = jwt_manager.GenerateToken(user_id, role, 24);
    
    // Try to verify with different secret
    JwtManager wrong_secret_manager("different_secret_key");
    auto payload = wrong_secret_manager.VerifyToken(token);
    
    EXPECT_FALSE(payload.has_value());
}

UTEST(JwtManager, VerifyToken_ExpiredToken) {
    JwtManager jwt_manager("test_secret_key_12345");
    
    const std::string user_id = "user123";
    const std::string role = common::kParticipantRole;
    
    // Generate token with very short expiration
    auto token = jwt_manager.GenerateToken(user_id, role, 0); // 0 hours = expired immediately
    
    // Wait a bit to ensure expiration
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto payload = jwt_manager.VerifyToken(token);
    
    EXPECT_FALSE(payload.has_value());
}

UTEST(JwtManager, GenerateAndVerify_RoundTrip) {
    JwtManager jwt_manager("test_secret_key_12345");
    
    const std::string user_id = "user123";
    const std::string role = common::kOrganizerRole;
    
    auto token = jwt_manager.GenerateToken(user_id, role, 24);
    auto payload = jwt_manager.VerifyToken(token);
    
    ASSERT_TRUE(payload.has_value());
    EXPECT_EQ(payload->user_id, user_id);
    EXPECT_EQ(payload->role, role);
}

UTEST(JwtManager, GenerateToken_CustomDuration) {
    JwtManager jwt_manager("test_secret_key_12345");
    
    const std::string user_id = "user123";
    const std::string role = common::kParticipantRole;
    
    auto token_1_hour = jwt_manager.GenerateToken(user_id, role, 1);
    auto token_7_days = jwt_manager.GenerateToken(user_id, role, 24 * 7);
    
    ASSERT_FALSE(token_1_hour.empty());
    ASSERT_FALSE(token_7_days.empty());
    EXPECT_NE(token_1_hour, token_7_days); // Different expiration times should produce different tokens
}

UTEST(JwtManager, GenerateToken_InvalidRole) {
    JwtManager jwt_manager("test_secret_key_12345");
    
    const std::string user_id = "user123";
    const std::string invalid_role = "invalid_role";
    
    EXPECT_THROW(jwt_manager.GenerateToken(user_id, invalid_role, 24), std::invalid_argument);
}

UTEST(JwtManager, VerifyToken_AllRoles) {
    JwtManager jwt_manager("test_secret_key_12345");
    
    const std::string user_id = "user123";
    
    // Test all valid roles
    for (const auto& role : {common::kParticipantRole, common::kOrganizerRole, common::kAdminRole}) {
        auto token = jwt_manager.GenerateToken(user_id, role, 24);
        auto payload = jwt_manager.VerifyToken(token);
        
        ASSERT_TRUE(payload.has_value());
        EXPECT_EQ(payload->user_id, user_id);
        EXPECT_EQ(payload->role, role);
    }
}
