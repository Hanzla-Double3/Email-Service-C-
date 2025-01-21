#ifndef DTOs_hpp
#define DTOs_hpp

#include "oatpp/macro/codegen.hpp"
#include "oatpp/Types.hpp"
#include "oatpp/web/protocol/http/Http.hpp"
#include "oatpp/macro/component.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

/**
 *  Data Transfer Object. Object containing fields only.
 *  Used in API for serialization/deserialization and validation
 */

/**
 *  email: from
 *  email: to
 *  subject:
 *  content:
 *  
 */



class MyDto : public oatpp::DTO {
  
  DTO_INIT(MyDto, DTO)
  
  DTO_FIELD(Int32, statusCode);
  DTO_FIELD(String, message);
  
};

class SendEmailDTO : public oatpp::DTO {
  DTO_INIT(SendEmailDTO, DTO)
  DTO_FIELD(oatpp::Fields<oatpp::String>, emailData);
  DTO_FIELD(oatpp::Fields<oatpp::String>, verificationData);
  DTO_FIELD(String, resp);
};


#include OATPP_CODEGEN_END(DTO)

#endif /* DTOs_hpp */