/**
 * @file logger.hpp
 * @brief Logger class declaration for YDLidar SDK.
 *
 * @version 0.1
 * @author Nikita Bulaev, Grovety Inc, agency@grovety.com
 * @copyright Copyright (c) 2024-2025, Vishvam corp.
 */
#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

/** @brief ydlidar */
namespace ydlidar {
/** @brief ydlidar core */
namespace core {
    namespace Log {
        enum Facility {
            EMERG   = 0, /*!< Emergency: system is unusable */
            ALERT   = 1, /*!< Alert: action must be taken immediately */
            CRIT    = 2, /*!< Critical: critical conditions */
            ERR     = 3, /*!< Error: error conditions */
            WARNING = 4, /*!< Warning: warning conditions */
            NOTICE  = 5, /*!< Notice: normal but significant condition */
            INFO    = 6, /*!< Informational: informational messages */
            DEBUG   = 7, /*!< Debug: debug-level messages */
        };

        class Logger {
          private:
            const std::string _ident;       /*!< Identifier for the log messages */
            std::atomic<enum Facility> _maxFacility; /*!< Maximum facility level to log */

          protected:
          public:
            Logger() = delete;

            /**
             * @brief Construct a new Logger object
             *
             * @param ident       Identifier for the log messages
             * @param maxFacility Maximum facility level to log
             */
            explicit Logger(const std::string &ident, enum Facility maxFacility);

            ~Logger();

            Logger(const Logger &) = delete;
            Logger &operator=(const Logger &) = delete;

            void msg(enum Facility facility, const std::string &msg);

            void emerg(const std::string &msg);
            void alert(const std::string &msg);
            void critical(const std::string &msg);
            void error(const std::string &msg);
            void warning(const std::string &msg);
            void notice(const std::string &msg);
            void info(const std::string &msg);
            void debug(const std::string &msg);

            /**
             * @brief Set maximum facility level to log
             *
             * @param facility Facility level
             */
            void setFacility(enum Facility facility);

            /**
             * @brief Convert facility level to string
             *
             * @param[in] facility Facility level
             *
             * @return std::string Facility level as string
             */
            static const std::string facilityToString(enum Facility facility);

            /**
             * @brief Get faciliti msg color
             *
             * @param[in] facility Facility level
             *
             * @return std::string Color code
             */
            static const std::string facilityToColor(enum Facility facility);
        };
    };  // namespace Log
}  // namespace core
}  // namespace ydlidar
