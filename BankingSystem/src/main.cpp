// Nova Bank - Banking & Management System (C++17 OOP mini project)
#include <iostream>
#include <cstring>
#include <cstdlib>
#include "App.h"

static void usage() {
    std::cout << "Nova Bank - Banking & Management System\n"
                 "Usage: bank [--data DIR] [--help]\n"
                 "  --data DIR   folder for CSV data (default: ./data, or $BANK_DATA_DIR)\n"
                 "Demo logins: customer SB1001/1234, CA2001/2345, FD3001/3456, ST1001/4567 | admin admin/admin123\n";
}

int main(int argc, char** argv) {
    std::string dataDir = std::getenv("BANK_DATA_DIR") ? std::getenv("BANK_DATA_DIR") : "data";
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--help") || !std::strcmp(argv[i], "-h")) { usage(); return 0; }
        if (!std::strcmp(argv[i], "--data") && i + 1 < argc) dataDir = argv[++i];
    }
    ui::init();
    Bank bank(dataDir);
    try {
        bank.load();
        App app(bank);
        app.run();
    } catch (const InputClosed&) {
        bank.saveAll();                      // stdin closed: persist and leave quietly
        std::cout << "\nInput closed. Data saved.\n";
    } catch (const BankException& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return 2;
    }
    return 0;
}
