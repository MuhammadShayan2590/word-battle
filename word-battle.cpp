#include <string>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
std::string word;
bool isValid() {
    std::unordered_set<std::string> words;
    std::ifstream file("words_alpha.txt");
    while (file >> word) words.insert(word);
    std::cout << "Enter word: ";
    std::cin >> word;
    std::transform(word.begin(), word.end(), word.begin(), ::tolower);
    if (words.find(word) != words.end()) {
        std::cout << "Valid English word\n";
        return true;
    }
    else {
        std::cout << "Not found in dictionary\n";
        return false;
    }
}
int main() {
    isValid();
}
