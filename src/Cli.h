// Cli: the TERMINAL layer. Shows menus, asks questions, prints results.
//
// Responsibility: talk to the person. It contains no library rules - it
// collects input, hands it to Library (rules) and Storage (files), and prints
// what comes back.
//
// Input handling is the same idea as the Expense Tracker: every answer is read
// with std::getline and validated by hand, so wrong input can never leave the
// stream in a broken state or cause an endless loop. Each question allows
// 3 invalid attempts and then cancels the operation. End-of-input (Ctrl+D /
// Ctrl+Z / closed pipe) ends the program cleanly.
#pragma once

#include <functional>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

#include "Date.h"
#include "Library.h"
#include "Storage.h"

class Cli {
public:
    Cli(Library& library, Storage& storage, const Date& today, std::istream& in,
        std::ostream& out);

    // Loads saved data, then runs the menu until the user exits or input ends.
    void run();

private:
    // Thrown (and caught inside run()) to abandon the current operation.
    struct InputEnded {};      // end-of-input: the program should exit
    struct InputCancelled {};  // too many invalid attempts: back to the menu

    enum class IdRule { MustExist, MustBeNew };

    // startup / shutdown
    void loadSavedData();
    void reportReadProblems(const std::string& fileName, const std::vector<RowError>& errors);
    void persist();

    // menu
    void showMenu();
    bool handleChoice(const std::string& line);  // false = exit

    // menu actions
    void listBooks();
    void searchBooks();
    void checkAvailability();
    void addBook();
    void updateBook();
    void addMember();
    void renameMember();
    void listMembers();
    void issueBook();
    void returnBook();
    void listLoans();
    void importCsv();

    // output helpers
    void printBooks(const std::vector<Book>& books);
    void printBookDetails(const Book& book);
    void report(const OperationResult& result);
    std::string statusOf(const std::string& bookId) const;

    // input helpers (throw InputEnded / InputCancelled)
    std::string readLine(const std::string& prompt);
    void askWithRetry(const std::string& prompt,
                      const std::function<bool(const std::string&, std::string&)>& accept);
    std::string askId(const std::string& prompt, const std::string& label,
                      const std::function<bool(const std::string&)>& exists, IdRule rule,
                      const std::string& hint);
    std::string askBookId(const std::string& prompt, IdRule rule);
    std::string askMemberId(const std::string& prompt, IdRule rule);
    std::string askText(const std::string& prompt, const std::string& label);
    std::optional<std::string> askOptionalText(const std::string& prompt, const std::string& label);
    int askYear(const std::string& prompt);
    std::optional<int> askOptionalYear(const std::string& prompt);

    Library& library_;
    Storage& storage_;
    Date today_;
    std::istream& in_;
    std::ostream& out_;
    bool unsavedChanges_ = false;  // true only if the last save attempt failed
};
