#ifndef MyController_hpp
#define MyController_hpp

#include "dto/DTOs.hpp"

#include "oatpp/web/protocol/http/outgoing/StreamingBody.hpp"
#include <fstream>

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"
#include "oatpp/utils/Conversion.hpp"
#include "EmailUtility/EmailHandler.cpp"
#include "database/databaseWrapper.cpp"

#define getInput(dto, obj, key, holder) \
    auto holder = dto->obj[key]; \
    if (!holder || holder->empty()) { \
        return createResponse(Status::CODE_400, "Invalid input data"); \
    }


#include OATPP_CODEGEN_BEGIN(ApiController) //<-- Begin Codegen

class MyController : public oatpp::web::server::api::ApiController
{
public:
  /**
   * Constructor with object mapper.
   * @param apiContentMappers - mappers used to serialize/deserialize DTOs.
   */
  MyController(OATPP_COMPONENT(std::shared_ptr<oatpp::web::mime::ContentMappers>, apiContentMappers))
      : oatpp::web::server::api::ApiController(apiContentMappers)
  {
  }

public:
  ENDPOINT("GET", "/image", image, REQUEST(std::shared_ptr<IncomingRequest>, request))
  {
    auto queryParams = request->getQueryParameters();
    auto id = queryParams.get("id");

    if(id){
      uint32_t intID = atoi(id.get()->c_str());

      TrackingDatabase *db = TrackingDatabase::getInstance();
      db->readMessage(intID);
    }

    auto buffer = oatpp::String::loadFromFile("image.png");

    auto response = createResponse(Status::CODE_200, buffer);
    response->putHeader(Header::CONTENT_TYPE, "image/jpeg");

    return response;
  }
  // TODO Insert Your endpoints here !!!
  ENDPOINT("POST", "/sentMail", swapKeysAndValues, BODY_DTO(Object<SendEmailDTO>, inputDto))
  {
    auto outputDto = SendEmailDTO::createShared();
    
    //verification
    bool verified = false;
    if (inputDto && inputDto->verificationData){
      getInput(inputDto,verificationData,"username", username);
      getInput(inputDto,verificationData,"password", password);
      if (username == Config::get("username") && password == Config::get("password")){
          verified = true;
      }
    }

    //send email
    if (verified && inputDto->emailData)
    {
      //tracking data
      std::string tracking_url = Config::get("tracking_url");
      uint32_t trackingId = 0;
      if (tracking_url == "default")
      {
        TrackingDatabase *db = TrackingDatabase::getInstance();
        trackingId = db->addMessage();
        tracking_url = Config::get("domain") + "/image?id=" + std::to_string(trackingId);
      }

      
      getInput(inputDto,emailData,"from", from);
      getInput(inputDto,emailData,"name", name);
      getInput(inputDto,emailData,"to", to);
      getInput(inputDto,emailData,"subject", subject);
      getInput(inputDto,emailData,"content", content);
      String resp = sendEmail(from, name, to, subject, content, tracking_url);

      outputDto->resp = resp;
      return createDtoResponse(Status::CODE_200, outputDto);
    }
    return createResponse(Status::CODE_400, "Invalid input data");
    
  }
};

#include OATPP_CODEGEN_END(ApiController) //<-- End Codegen

#endif /* MyController_hpp */