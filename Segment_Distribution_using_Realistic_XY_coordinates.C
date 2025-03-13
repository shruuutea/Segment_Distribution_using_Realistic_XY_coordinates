#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <TROOT.h>
#include <TH2.h>
#include <TPolyLine.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TLegend.h>

using namespace std;

// Channel Number Parser
vector<int> parseMuchChanCount(const string& filename) {
    vector<int> muchChanCount;
    ifstream file(filename);
    string line, content;
    
    // Read entire file into memory
    while (getline(file, line)) content += line + "\n";

    // Extract the muchChanCount array from file content
    size_t start = content.find("muchChanCount=[");
    if (start == string::npos) return muchChanCount;
    start += 15; // Skip past the array declaration
    size_t end = content.find("]", start);
    string arrayStr = content.substr(start, end - start);

    // Convert comma-separated string to integer vector
    stringstream ss(arrayStr);
    string token;
    while (getline(ss, token, ',')) {
        token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
        if (!token.empty()) muchChanCount.push_back(stoi(token));
    }
    return muchChanCount;
}

// Coordinate File Reader
vector<pair<double, double>> readXYCoordinates(const string& filename) {
    vector<pair<double, double>> coordinates;
    ifstream file(filename);
    string line;
    getline(file, line); // Skip header line

    // Read exactly 1824 coordinate pairs
    for (int i = 0; i < 1824; ++i) {
        if (!getline(file, line)) break;
        replace(line.begin(), line.end(), ',', ' '); // Convert CSV to space-separated
        stringstream ss(line);
        double x, y;
        if (ss >> x >> y) coordinates.push_back({x, y});
    }
    return coordinates;
}

void Segment_Distribution_using_Realistic_XY_coordinates(const string& chan_file = "uniqueID.txt",        //MuChChanCount file
                   const string& coord_file = "abc.csv") {     //abc.csv file which contains realistic XY coordinates of 2nd Station GEM Module
    // Data Loading
    vector<int> muchChanCount = parseMuchChanCount(chan_file);
    vector<pair<double, double>> coordinates = readXYCoordinates(coord_file);

    // Validate data integrity
    if (muchChanCount.size() != 1824 || coordinates.size() != 1824) {
        cerr << "Data size mismatch! muchChanCount: " << muchChanCount.size() 
             << ", coordinates: " << coordinates.size() << endl;
        return;
    }

    // Canvas Preparation
    TCanvas *c1 = new TCanvas("c1", "GEM Readout Pads - Channel Groups", 1100, 1100);
    c1->SetGrid(1, 1);  // Enable grid lines
    gStyle->SetOptStat(0);  // Disable statistics box

    // Configuration & Setup
    const int pads_per_layer = 19;  // Pads per detector layer
    vector<int> colors = {  // 15 distinct colors for channel groups
        kRed, kBlue, kGreen, kMagenta, kCyan, kYellow, kOrange,
        kPink, kSpring, kTeal, kAzure, kViolet, kGray, kOrange+7, kGreen+3
    };
    vector<TPolyLine*> pads;  // Store all pad polygons
    double x_min = 1e9, x_max = -1e9, y_min = 1e9, y_max = -1e9;  // Plot boundaries

    // Pad Creation Loop
    for (size_t i = 0; i < 1824; ++i) {
        int chan = muchChanCount[i];
        if (chan == -9 || chan < 1 || chan > 1920) continue;
        if (i >= 1824 - pads_per_layer) continue;  // Skip last pad of the last layer

        bool right_edge = (i + 1) % pads_per_layer == 0;

        // Coordinate Calculation
        double x[5], y[5];
        if (!right_edge) {
            x[0] = coordinates[i].first;
            y[0] = coordinates[i].second;
            x[1] = coordinates[i + 1].first;
            y[1] = coordinates[i + 1].second;
            x[2] = coordinates[i + pads_per_layer + 1].first;
            y[2] = coordinates[i + pads_per_layer + 1].second;
            x[3] = coordinates[i + pads_per_layer].first;
            y[3] = coordinates[i + pads_per_layer].second;
        } else {
            // Modify transition for rightmost pads
            double x_offset = coordinates[i].first - coordinates[i - 1].first;
            double y_offset = coordinates[i].second - coordinates[i - 1].second;

            x[0] = coordinates[i].first;
            y[0] = coordinates[i].second;
            x[1] = coordinates[i].first + x_offset;
            y[1] = coordinates[i].second + y_offset;
            x[2] = coordinates[i + pads_per_layer].first + x_offset;
            y[2] = coordinates[i + pads_per_layer].second + y_offset;
            x[3] = coordinates[i + pads_per_layer].first;
            y[3] = coordinates[i + pads_per_layer].second;
        }
        x[4] = x[0];  // Close polygon
        y[4] = y[0];

        // Color Assignment
        int group = (chan - 1) / 128;  // 128 channels per group
        int color = colors[group % 15];

        // Create and style pad
        TPolyLine *pad = new TPolyLine(5, x, y);
        pad->SetFillColor(color);
        pad->SetLineColor(kBlack);
        pad->SetLineWidth(1);
        pads.push_back(pad);

        // Update plot boundaries
        x_min = min({x_min, x[0], x[1], x[2], x[3]});
        x_max = max({x_max, x[0], x[1], x[2], x[3]});
        y_min = min({y_min, y[0], y[1], y[2], y[3]});
        y_max = max({y_max, y[0], y[1], y[2], y[3]});
    }

    // Frame & Drawing
    TH2F *frame = new TH2F("frame", "2nd Station GEM Segment Distribution using Realistic XY coordinates;X (mm);Y (mm)", 
                           10, x_min - 10, x_max + 10, 10, y_min - 10, y_max + 10);
    frame->Draw();

    // Draw all pads
    for (auto pad : pads) {
        pad->Draw("f");
        pad->Draw("l");
    }

    // Legend with colored boxes
    TLegend *leg = new TLegend(0.92, 0.1, 0.99, 0.9);
    leg->SetHeader("MuchChanCount and their corresponding Connector Region", "C");
	leg->SetTextSize(0.01);
    for (int i = 0; i < 15; ++i) {
        TPolyLine *dummy = new TPolyLine();  // Dummy polyline for legend
        dummy->SetFillColor(colors[i]);
        dummy->SetLineColor(kBlack);
       // leg->AddEntry(dummy, Form("%d-%d", i * 128 + 1, (i + 1) * 128), "f");
	 //leg->AddEntry(dummy, Form("Connector %d", i + 1), "f");
	leg->AddEntry(dummy, Form("%d-%d, Connector Region %d", i * 128 + 1, (i + 1) * 128, i + 1), "f");

    }
    leg->Draw();

    // Save output
    c1->SaveAs("gem_channel_groups_fixed.png");
    cout << "Saved gem_channel_groups_fixed.png with fixed legend." << endl;
}

