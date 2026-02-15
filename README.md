# eps

```mermaid
graph TD
    A[/"Clients"/] --> B("(Auth)")
    B --> C["Monolith (Python App)"]
    C <--> D["REST API (C++)"]
    D <--> E[("MySQL Database")]
    D <--> F[("Redis (Optional)")]

    subgraph Optional
        G{"Load Balancer: Clients - reverse proxy / proxy  <-> Monolith"}
    end

    B --> G
    C --> A
    G <--> C
    G --> A

    subgraph "Monolith Services"
        C1[["Doc Serializer (XLS, PDF, HTML -> JSON) <br>(PDF) Input: 689 KB Output: 8 KB Time: 2.5641 s <br>(HTM) Input: 44 KB Output: 20 KB Time: 0.0300 s"]]
        C2[["Document Parser <br>Input: 20 KB <br>Output: 25 KB <br>Time: 0.0005 s"]]
    end

    C --> C1
    C --> C2

    %% Metrics as comments and annotations
    C1 --> M1("Serialize a pdf in 2.50s ")
    M1 --> M2("Stores processed JSON only (no raw files)")

    C2 --> M3("No metric but load is of no consequences")
    M3 --> M4("Return serialized e-mails to the client in a json format")   


    %% Assign classes
    class A client;
    class B auth;
    class C monolith;
    class D api;
    class E,F db;
    class G proxy;
    class C1,C2,C3,C4 service;
    class M1,M2,M3,M4,M5,M6,M7,M8 metric;

    %% Define styles
    classDef client fill:#e3f2fd,stroke:#2196f3,stroke-width:2px,color:#000;
    classDef auth fill:#fff3e0,stroke:#fb8c00,stroke-width:2px,color:#000;
    classDef monolith fill:#ede7f6,stroke:#673ab7,stroke-width:2px,color:#000;
    classDef api fill:#e0f7fa,stroke:#00acc1,stroke-width:2px,color:#000;
    classDef db fill:#f1f8e9,stroke:#558b2f,stroke-width:2px,color:#000;
    classDef proxy fill:#fce4ec,stroke:#ec407a,stroke-width:2px,color:#000;
    classDef service fill:#f3e5f5,stroke:#9c27b0,stroke-width:1.5px,color:#000;
    classDef metric fill:#f5f5f5,stroke:#9e9e9e,stroke-dasharray: 5 5,color:#000;
```

### Data Highlights

| Component            | Input  | Output | Time     |
| -------------------- | ------ | ------ | -------- |
| Doc Serializer (PDF) | 689 KB | 8 KB   | 2.5641 s |
| Doc Serializer (HTM) | 44 KB  | 20 KB  | 0.0300 s |
| Document Parser      | 20 KB  | 25 KB  | 0.0005 s |


### Color Roles

| Role          | Background     | Border      |
| ------------- | -------------- | ----------- |
| Clients       | Light Blue     | Blue        |
| Auth          | Light Orange   | Deep Orange |
| Monolith      | Light Purple   | Indigo      |
| REST API      | Aqua           | Cyan        |
| Databases     | Light Green    | Green       |
| Proxy/LB      | Pink           | Rose        |
| Internal Svcs | Light Lavender | Purple      |
| Metrics       | Light Gray     | Dashed Gray |


### Custom Shapes Legend

| Shape           | Component Type             |
| --------------- | -------------------------- |
| `rect`          | General Services / App     |
| `cylinder`      | Databases (MySQL, Redis)   |
| `parallelogram` | Client Input/Output        |
| `hexagon`       | Load Balancer / Proxy      |
| `roundrect`     | Auth / Identity            |
| `subroutine`    | Internal Monolith Services |

* **`/"..."`** → Parallelogram (Client)
* **`[...]`** → Rectangle (Monolith/API)
* **`[(...)]`** → Cylinder (Database)
* **`[[...]]`** → Subroutine (Internal service)
* **`{{...}}`** → Hexagon (Proxy/Load Balancer)
* **`(...)`** → Roundrect (Auth, metrics)


***
