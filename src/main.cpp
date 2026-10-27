// Library Manager - entry point. Only wires the three layers together:
//   data (Storage)  +  business rules (Library)  +  terminal (Cli)
//
// Usage:  library_manager [--data-dir <folder>] [--today YYYY-MM-DD] [--help]
//   --data-dir   where books.csv / members.csv are kept (default: library_data)
//   --today      pretend today is this date (handy for testing due dates)

#include <iostream>
#include <optional>
#include <string>

#include "Cli.h"
#include "Date.h"
#include "Library.h"
#include "Storage.h"

namespace {

void printUsage(std::ostream& out) {
    out << "Usage: library_manager [--data-dir <folder>] [--today YYYY-MM-DD] [--help]\n"
        << "  --data-dir <folder>   folder for books.csv and members.csv (default: library_data)\n"
        << "  --today YYYY-MM-DD    use this date instead of the computer's date\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string dataDir = "library_data";
    Date today = Date::today();

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(std::cout);
            return 0;
        }
        if ((arg == "--data-dir" || arg == "--today") && i + 1 >= argc) {
            std::cerr << "Error: " << arg << " needs a value.\n";
            printUsage(std::cerr);
            return 2;
        }
        if (arg == "--data-dir") {
            dataDir = argv[++i];
        } else if (arg == "--today") {
            const std::optional<Date> parsed = Date::parse(argv[++i]);
            if (!parsed) {
                std::cerr << "Error: --today must be a real date written as YYYY-MM-DD.\n";
                return 2;
            }
            today = *parsed;
        } else {
            std::cerr << "Error: unknown option '" << arg << "'.\n";
            printUsage(std::cerr);
            return 2;
        }
    }

    Library library;
    Storage storage(dataDir);
    Cli cli(library, storage, today, std::cin, std::cout);
    cli.run();
    return 0;
}
