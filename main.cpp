#include <iostream>
#include <string>
#include <sstream>
#include <curl/curl.h>
#include <json/json.h>
#include <thread>
#include <vector>

// Callback function to write CURL response data into a string
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t totalSize = size * nmemb;
    s->append((char*)contents, totalSize);
    return totalSize;
}

// Function to fetch weather data from OpenWeatherMap API
std::string fetchWeatherData(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    else {
        std::cerr << "Failed to initialize CURL" << std::endl;
    }

    return readBuffer;
}

// Function to parse coordinates from the JSON response
bool parseCoordinates(const std::string& jsonData, double& lat, double& lon) {
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;

    std::istringstream s(jsonData);
    if (!Json::parseFromStream(readerBuilder, s, &root, &errs)) {
        std::cerr << "Failed to parse JSON Data: " << errs << std::endl;
        return false;
    }

    //getting coordinates of the city entered 
    if (root.isMember("coord")) {
        lat = root["coord"]["lat"].asDouble();
        lon = root["coord"]["lon"].asDouble();
        return true;
    }
    else {
        std::cerr << "Coordinates not found in JSON response" << std::endl;
        return false;
    }
}

// Function to parse and print 5-day forecast from the JSON response
void parseAndPrintForecast(const std::string& jsonData) {
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;

    std::istringstream s(jsonData);
    if (!Json::parseFromStream(readerBuilder, s, &root, &errs)) {
        std::cerr << "Failed to parse JSON: " << errs << std::endl;
        return;
    }

    if (root.isMember("list")) {
        const Json::Value& list = root["list"];

        //gets the 5 day data
        for (Json::Value::ArrayIndex i = 0; i < list.size(); i += 8) {
            std::cerr << "****Weather *****" << std::endl;
            std::string description;
            std::cerr << "Date & Time: " << list[i]["dt_txt"] << std::endl;
            const auto& item = list[i]["weather"];
            std::cerr << "Weather: " << item[0]["description"] << std::endl;
            double temperature = list[i]["main"]["temp"].asDouble();
            temperature = temperature - 273.15;
            std::cout << "Temperature: " << temperature << "°C" << std::endl;
        }
    }
    else {
        std::cerr << "No 'list' member in JSON response" << std::endl;
    }
}

void fetchWeatherForCity(const std::string& city, const std::string& apiKey) {
    try {
        //Gets the coordinates of the city
        std::string coordUrl = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + apiKey;
        std::string coordData = fetchWeatherData(coordUrl);

        if (coordData.empty()) {
            std::cerr << "Failed to get data for coordinates" << std::endl;
            return;
        }

        double lat, lon;
        if (!parseCoordinates(coordData, lat, lon)) {
            return;
        }
        else {
            std::string forecastUrl = "http://api.openweathermap.org/data/2.5/forecast?lat=" + std::to_string(lat) + "&lon=" + std::to_string(lon) + "&appid=" + apiKey;

            // Gets the 5-day weather forecast using the coordinates
            std::string forecastData = fetchWeatherData(forecastUrl);

            if (forecastData.empty()) {
                std::cerr << "Failed to get data for forecast" << std::endl;
                return;
            }
            else {
                parseAndPrintForecast(forecastData);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
    }
}

int main() {
    std::string city;
    std::string apiKey = "db751ef0e105d932448706aa58e7dab3"; // API key

    std::cout << "Enter a city name to get weather information: ";
    std::getline(std::cin, city);

    if (!city.empty()) {

        //a thread to fetch weather info for the city
        std::thread weatherThread(fetchWeatherForCity, city, apiKey);
        weatherThread.join();

    }
    else {
        std::cout << "City name cannot be empty." << std::endl;
    }

    return 0;
}
