#include <iostream>
#include <fstream>
#include <vector>
#include <thread>    
#include <mutex>      
#include <chrono>  

using namespace std;
using namespace std::chrono;

mutex mtx;

// Run-Length Encoding Compression
string compressChunk(const string& data) {
    string compressed = "";
    int n = data.size();
    for (int i = 0; i < n; i++) {
        int count = 1;
        while (i + 1 < n && data[i] == data[i + 1]) {
            count++;
            i++;
        }
        compressed += data[i] + to_string(count);
    }
    return compressed;
}

// Decompression
string decompressChunk(const string& data) {
    string decompressed = "";
    int n = data.size();
    for (int i = 0; i < n; i++) {
        char ch = data[i];
        string countStr = "";
        while (i + 1 < n && isdigit(data[i + 1])) {
            countStr += data[++i];
        }
        int count = stoi(countStr);
        decompressed += string(count, ch);
    }
    return decompressed;
}

// Multithreading wrapper
void threadCompress(const string& chunk, string& result) {
    string localResult = compressChunk(chunk);
    lock_guard<mutex> lock(mtx);
    result += localResult;
}

void threadDecompress(const string& chunk, string& result) {
    string localResult = decompressChunk(chunk);
    lock_guard<mutex> lock(mtx);
    result += localResult;
}

void splitData(const string& data, vector<string>& chunks, int threads) {
    size_t chunkSize = data.size() / threads;
    size_t start = 0;
    for (int i = 0; i < threads; i++) {
        size_t end = (i == threads - 1) ? data.size() : start + chunkSize;
        chunks.push_back(data.substr(start, end - start));
        start = end;
    }
}

int main() {
    string filename, choice;
    cout << "Enter file name (with .txt): ";
    cin >> filename;

    ifstream inputFile(filename);
    if (!inputFile) {
        cerr << "File not found.\n";
        return 1;
    }

    string data((istreambuf_iterator<char>(inputFile)), istreambuf_iterator<char>());
    inputFile.close();

    int threadCount;
    cout << "Enter number of threads: ";
    cin >> threadCount;

    cout << "Choose operation (compress / decompress): ";
    cin >> choice;

    string result = "";
    vector<string> chunks;
    vector<thread> threads;

    splitData(data, chunks, threadCount);

    auto start = high_resolution_clock::now();

    if (choice == "compress") {
        for (int i = 0; i < threadCount; i++) {
            threads.push_back(thread(threadCompress, chunks[i], ref(result)));
        }
    } else if (choice == "decompress") {
        for (int i = 0; i < threadCount; i++) {
            threads.push_back(thread(threadDecompress, chunks[i], ref(result)));
        }
    } else {
        cout << "Invalid choice.\n";
        return 1;
    }

    for (auto& t : threads) t.join();

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    string outFile = (choice == "compress") ? "compressed.txt" : "decompressed.txt";
    ofstream outputFile(outFile);
    outputFile << result;
    outputFile.close();

    cout << "Operation complete. Output written to: " << outFile << endl;
    cout << "Time taken with " << threadCount << " threads: " << duration.count() << " ms\n";

    return 0;
}