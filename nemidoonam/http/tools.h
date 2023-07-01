#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "HTTPRequest.hpp"
#include "json.hpp"

class absChat;
class Chat;
class Group;
class Channel;
class Message;
class User;


nlohmann::json http_get(std::map<std::string, std::string>&);
nlohmann::json http_get(const std::string&, std::map<std::string, std::string>&);

struct Message
{
    std::string body;
    std::string date;
    std::string src;
    std::string dst;
    Message() {}
    Message(nlohmann::json);
    Message(const std::string&, const std::string&, const std::string&, const std::string&);
    friend std::istream& operator>>(std::istream& input, Message& obj) {
        input >> obj.body >> obj.date >> obj.src >> obj.dst;
        return input;
    }
};

class absChat
{
public:
    absChat(const std::string& _id): id(_id) {}
    int messageNum() const;
    std::vector<Message*> getMessages();
    void addMessage(Message*);
    const std::string getID() const;
    void setID(const std::string&);
    bool operator<(const absChat&);
    virtual ~absChat() = 0;
protected:
    std::string id; // contact or group id (dst)
    std::vector<Message*> messages;
};

class Chat: public absChat
{
public:
    Chat(const std::string& _id): absChat(_id) {}
    static std::string stringView() { return "user"; }
};

class Group: public absChat
{
public:
    Group(const std::string& _id): absChat(_id) {}
    static std::string stringView() { return "group"; }
};

class Channel: public absChat
{
public:
    Channel(const std::string& _id): absChat(_id) {}
    void setAdmin(const std::string&);
    const std::string& getAdmin();
    static std::string stringView() { return "channel"; }
private:
    std::string admin_id; // server doesn't support lol
};

class User
{
public:
    // static bool exist; // singleton ??
    User();
    void retrieveServer();
    template<typename T> void retrieveChat(absChat* chat);

    void signup(const std::string&, const std::string&);
    void login(const std::string&, const std::string&);
    void logout();

    const std::string getToken();
    const std::string getUser();
    std::vector<absChat*> getChats();
    void addChat(absChat*);

    template<typename T> T* joinChannel(const std::string& id);
    template<typename T> T* createChat(const std::string& id);
    template<typename T> void sendMessage(const std::string& body, absChat* dst);

    ~User();
private:
    bool is_loggedin;
    std::string token;
    std::string username;
    std::string password;
    std::vector<absChat*> chats; // change it to set??
};

template<typename T> void User::retrieveChat(absChat* chat)
{
    std::map<std::string, std::string> mm {{"token", token}, {"dst", chat->getID()}};
    nlohmann::json res = http_get("get" + T::stringView() + "chats", mm);
    if (res["code"] != "200") {
        throw std::runtime_error("unexpected error");
    }
    for (int i=chat->messageNum();i<res.size()-2;i++) chat->addMessage(new Message(res["block " + std::to_string(i)]));
}

template<typename T> void User::sendMessage(const std::string& body, absChat* dst)
{
    std::string s = T::stringView();

    std::map<std::string, std::string> mm {{"token", token}, {"body", body}, {"dst", dst->getID()}};
    nlohmann::json res = http_get("sendmessage" + s, mm);
    if (res["code"] != "200") {
        throw std::runtime_error(res["message"]);
    }

    retrieveChat<T>(dst);
}

template<typename T> T* User::joinChannel(const std::string& id)
{
    std::string s = T::stringView();

    std::map<std::string, std::string> mm {{"token", token}, {s + "_name", id}};
    nlohmann::json res = http_get("join" + s, mm);
    if (res["code"]!="200"){
        throw std::runtime_error(res["message"]);
    }

    T* new_chat = new T(id);
    retrieveChat<T>(new_chat);
    addChat(new_chat);
    return new_chat;
}

template<typename T> T* User::createChat(const std::string& id)
{
    std::string s = T::stringView();

    std::map<std::string, std::string> mm {{"token", token}, {s + "_name", id}};
    nlohmann::json res = http_get("create" + s, mm);
    if (res["code"]!="200"){
        throw std::runtime_error(res["message"]);
    }

    T* new_chat = new T(id);
    addChat(new_chat);
    return new_chat;
}
