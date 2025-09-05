#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <queue>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include <memory>

using namespace std;

class Request {
public:
    string id;
    string requestType;
    unordered_map<string, string> parameters; // Extra metadata

    void display() {
        cout << "Request ID: " << id << "\nType: " << requestType << "\nParameters:\n";
        for (auto& p : parameters) {
            cout << "  - " << p.first << ": " << p.second << "\n";
        }
    }
};

class Destination {
public:
    string ipAddress;
    int requestsBeingServed = 0;
    int threshold;

    Destination(string ip, int thresh)
        : ipAddress(ip), threshold(thresh) {}

    bool acceptRequest(Request& request) {
        if (requestsBeingServed < threshold) {
            requestsBeingServed++;
            cout << "✅ Request accepted by " << ipAddress << ". Currently serving: "
                 << requestsBeingServed << " requests (Threshold: " << threshold << ").\n";
            return true;
        }
        cout << "❌ Request rejected by " << ipAddress << " (overloaded).\n";
        return false;
    }

    void completeRequest() {
        if (requestsBeingServed > 0) {
            requestsBeingServed--;
            cout << "⚡ Request completed by " << ipAddress << ". Currently serving: "
                 << requestsBeingServed << " requests.\n";
        } else {
            cout << "No active requests on " << ipAddress << ".\n";
        }
    }
};

class Service {
public:
    string name;
    set<Destination*> destinations;

    void addDestination(Destination* destination) {
        destinations.insert(destination);
    }

    void removeDestination(Destination* destination) {
        destinations.erase(destination);
    }
};

class LoadBalancer {
protected:
    unordered_map<string, Service*> serviceMap;

public:
    void registerService(const string& requestType, Service* service) {
        serviceMap[requestType] = service;
    }

    set<Destination*>& getDestinations(Request& request) {
        if (serviceMap.find(request.requestType) == serviceMap.end()) {
            throw runtime_error("No service found for the request type.");
        }
        return serviceMap[request.requestType]->destinations;
    }

    virtual Destination* balanceLoad(Request& request) = 0;
};

class LeastConnectionLoadBalancer : public LoadBalancer {
public:
    Destination* balanceLoad(Request& request) override {
        auto& destinations = getDestinations(request);
        if (destinations.empty()) throw runtime_error("No destinations available.");

        return *min_element(destinations.begin(), destinations.end(),
                            [](Destination* a, Destination* b) {
                                return a->requestsBeingServed < b->requestsBeingServed;
                            });
    }
};

class RoutedLoadBalancer : public LoadBalancer {
public:
    Destination* balanceLoad(Request& request) override {
        auto& destinations = getDestinations(request);
        if (destinations.empty()) throw runtime_error("No destinations available.");

        vector<Destination*> list(destinations.begin(), destinations.end());
        size_t index = hash<string>{}(request.id) % list.size();
        return list[index];
    }
};

class RoundRobinLoadBalancer : public LoadBalancer {
private:
    unordered_map<string, queue<Destination*>> destinationQueues;

public:
    Destination* balanceLoad(Request& request) override {
        auto& destinations = getDestinations(request);
        if (destinations.empty()) throw runtime_error("No destinations available.");

        if (destinationQueues.find(request.requestType) == destinationQueues.end()) {
            queue<Destination*> q;
            for (Destination* dest : destinations) q.push(dest);
            destinationQueues[request.requestType] = q;
        }

        Destination* destination = destinationQueues[request.requestType].front();
        destinationQueues[request.requestType].pop();
        destinationQueues[request.requestType].push(destination);
        return destination;
    }
};

int main() {
    // Setup services and destinations
    Service service;
    Destination dest1{"192.168.0.1", 3};
    Destination dest2{"192.168.0.2", 2};
    Destination dest3{"192.168.0.3", 4};

    service.addDestination(&dest1);
    service.addDestination(&dest2);
    service.addDestination(&dest3);

    // Setup Load Balancers
    LeastConnectionLoadBalancer leastConnectionLB;
    RoutedLoadBalancer routedLB;
    RoundRobinLoadBalancer roundRobinLB;

    leastConnectionLB.registerService("http", &service);
    routedLB.registerService("http", &service);
    roundRobinLB.registerService("http", &service);

    LoadBalancer* lb = &leastConnectionLB; // Default

    while (true) {
        cout << "\n=== Load Balancer Simulation ===\n";
        cout << "1: Choose Least Connection\n";
        cout << "2: Choose Routed\n";
        cout << "3: Choose Round Robin\n";
        cout << "4: Send Request\n";
        cout << "5: Complete Request (manually)\n";
        cout << "6: Adjust Destination Threshold\n";
        cout << "7: Exit\n";
        cout << "Choose: ";
        int choice;
        cin >> choice;

        if (choice == 7) break;

        switch (choice) {
            case 1:
                lb = &leastConnectionLB;
                cout << "Switched to Least Connection LB.\n";
                break;
            case 2:
                lb = &routedLB;
                cout << "Switched to Routed LB.\n";
                break;
            case 3:
                lb = &roundRobinLB;
                cout << "Switched to Round Robin LB.\n";
                break;
            case 4: {
                Request request;
                string id;
                cout << "Enter request ID: ";
                cin >> id;
                request.id = "REQ" + id;
                request.requestType = "http";

                // Example metadata
                request.parameters["Resolution"] = "1080p";
                request.parameters["Format"] = "MP4";
                request.parameters["Priority"] = "High";

                try {
                    Destination* destination = lb->balanceLoad(request);
                    if (destination->acceptRequest(request)) {
                        cout << "Request routed to: " << destination->ipAddress << "\n";
                        request.display();
                    }
                } catch (const exception& e) {
                    cout << "Error: " << e.what() << "\n";
                }
                break;
            }
            case 5: {
                string ip;
                cout << "Enter destination IP to complete request: ";
                cin >> ip;
                if (ip == dest1.ipAddress) dest1.completeRequest();
                else if (ip == dest2.ipAddress) dest2.completeRequest();
                else if (ip == dest3.ipAddress) dest3.completeRequest();
                else cout << "No such destination.\n";
                break;
            }
            case 6: {
                string ip;
                int newThresh;
                cout << "Enter destination IP: ";
                cin >> ip;
                cout << "Enter new threshold: ";
                cin >> newThresh;
                if (ip == dest1.ipAddress) dest1.threshold = newThresh;
                else if (ip == dest2.ipAddress) dest2.threshold = newThresh;
                else if (ip == dest3.ipAddress) dest3.threshold = newThresh;
                else cout << "No such destination.\n";
                cout << "Threshold updated for " << ip << ".\n";
                break;
            }
            default:
                cout << "Invalid choice. Try again.\n";
        }
    }

    return 0;
}
