#include <oatpp/Types.hpp>

#include <vmime/vmime.hpp>
#include <vmime/platforms/posix/posixHandler.hpp>

#include "config/config.h"
#include "fstream"


typedef std::string String;


class JustAcceptIt : public vmime::security::cert::defaultCertificateVerifier{
public:
    void verify(
        const vmime::shared_ptr <vmime::security::cert::certificateChain>& chain,
		const vmime::string& hostname){
        return;
    }
};


void appendTrackingImage(std::string &content, const std::string &trackingUrl){
    size_t bodyPos = content.find("</body>");
    // Insert the tracking image before the closing </body> tag
    if (bodyPos != std::string::npos) {
        content.insert(bodyPos, trackingUrl);
    }
    else{
        size_t htmlPos = content.find("</html>");
        if (htmlPos != std::string::npos) {
            // Insert the tracking image before the closing </html> tag
            content.insert(htmlPos, trackingUrl);
        } else {
            std::string imageTag = "<img src=\"" + trackingUrl + "\">";
            // Append the tracking image to the end
            content.append(imageTag);
        }
    }

}


String sendEmail(String from, String name, String to, String subject, String content, String trackingUrl = "") {
    try {
        vmime::messageBuilder mb;

        // filling headers
        mb.setSubject(vmime::text(subject));
        vmime::mailbox sender(vmime::text(name), from);
        mb.setExpeditor(sender);
        mb.getRecipients().appendAddress(vmime::make_shared<vmime::mailbox>(to));

        mb.constructTextPart(vmime::mediaType(vmime::mediaTypes::TEXT, vmime::mediaTypes::TEXT_HTML));

        vmime::shared_ptr <vmime::htmlTextPart> textPart = 
            vmime::dynamicCast <vmime::htmlTextPart>(mb.getTextPart());

        mb.getTextPart()->setCharset(vmime::charsets::ISO8859_15);

        if (!trackingUrl.empty()){
            appendTrackingImage(content, trackingUrl);
        }

        mb.getTextPart()->setText(vmime::make_shared<vmime::stringContentHandler>(content));

        vmime::shared_ptr <vmime::message> msg = mb.construct();
        
        vmime::shared_ptr<vmime::net::session> sess = vmime::net::session::create();
        
        std::string smtp_server = Config::get("email_host");
        
        vmime::utility::url url(smtp_server); 
        vmime::shared_ptr<vmime::net::transport> tr = sess->getTransport(url);

        tr->setProperty("connection.tls", true);
        tr->setProperty("options.need-authentication", true);
        tr->setCertificateVerifier(vmime::make_shared<JustAcceptIt>());

        std::string email = Config::get("email");
        tr->setProperty("auth.username", email);

        std::string password = Config::get("email_password");
        tr->setProperty("auth.password", password); 
        tr->connect();
        tr->send(msg);

        tr->disconnect();

        return "Email sent successfully";
    } catch (vmime::exception& e) {
        return String("Error sending email: ") + String(e.what());
    } catch (std::exception& e) {
        return String("Standard exception: ") + String(e.what());
    }
}
