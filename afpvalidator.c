#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define SF_INTRODUCER 0x5A

// AFP architecture components
typedef enum {
    COMPONENT_UNKNOWN,
    COMPONENT_DOCUMENT,
    COMPONENT_PAGE_GROUP,
    COMPONENT_PAGE,
    COMPONENT_OBJECT,
    COMPONENT_RESOURCE_GROUP,
    COMPONENT_OVERLAY,
    COMPONENT_RESOURCE
} AFPComponent;

// AFP data object types
typedef enum {
    OBJ_UNKNOWN,
    OBJ_PRESENTATIONTEXT,
    OBJ_IMAGE,
    OBJ_GRAPHICS,
    OBJ_BARCODE,
    OBJ_FONT,
    OBJ_PAGSEG,
    OBJ_FORMDEF,
    OBJ_RESLIB
} AFPObjectType;

typedef struct {
    uint16_t length;
    unsigned char type[3];
    unsigned char flags;
    unsigned char *data;
    AFPComponent component;
    AFPObjectType obj_type;
    char name[9]; // Resource/Page name (null-terminated)
} StructuredField;

// Stack for tracking document structure nesting
#define MAX_STACK_SIZE 100
typedef struct {
    AFPComponent components[MAX_STACK_SIZE];
    int top;
} ComponentStack;

// Initialize stack
void stack_init(ComponentStack *stack) {
    stack->top = -1;
}

// Push component to stack
bool stack_push(ComponentStack *stack, AFPComponent component) {
    if (stack->top >= MAX_STACK_SIZE - 1)
        return false;
    stack->components[++(stack->top)] = component;
    return true;
}

// Pop component from stack
AFPComponent stack_pop(ComponentStack *stack) {
    if (stack->top < 0)
        return COMPONENT_UNKNOWN;
    return stack->components[(stack->top)--];
}

// Peek at top component
AFPComponent stack_peek(ComponentStack *stack) {
    if (stack->top < 0)
        return COMPONENT_UNKNOWN;
    return stack->components[stack->top];
}

// Is stack empty
bool stack_empty(ComponentStack *stack) {
    return stack->top < 0;
}

// Function to identify field type
void identify_field_type(StructuredField *field) {
    // Initialize with defaults
    field->component = COMPONENT_UNKNOWN;
    field->obj_type = OBJ_UNKNOWN;
    
    // Begin/End structures
    if (field->type[0] == 0xD3) {
        if (field->type[1] == 0xA8) {
            switch (field->type[2]) {
                case 0xC6: // BDT
                    field->component = COMPONENT_DOCUMENT;
                    break;
                case 0xC8: // EDT
                    field->component = COMPONENT_DOCUMENT;
                    break;
                case 0xAF: // BPG
                    field->component = COMPONENT_PAGE;
                    break;
                case 0xD8: // EPG
                    field->component = COMPONENT_PAGE;
                    break;
                case 0xD6: // BNG
                    field->component = COMPONENT_PAGE_GROUP;
                    break;
                case 0xD9: // ENG
                    field->component = COMPONENT_PAGE_GROUP;
                    break;
                case 0xA8: // BAG
                    field->component = COMPONENT_OBJECT;
                    break;
                case 0xA9: // EAG
                    field->component = COMPONENT_OBJECT;
                    break;
                case 0xDF: // BRG
                    field->component = COMPONENT_RESOURCE_GROUP;
                    break;
                case 0xE3: // ERG
                    field->component = COMPONENT_RESOURCE_GROUP;
                    break;
                case 0xBB: // BMO
                    field->component = COMPONENT_OVERLAY;
                    break;
                case 0xEB: // EMO
                    field->component = COMPONENT_OVERLAY;
                    break;
            }
        }
    }
    
    // Data objects and resources
    else if (field->type[0] == 0xD3) {
        // Presentation Text
        if (field->type[1] == 0xEE && field->type[2] == 0xBB) {
            field->obj_type = OBJ_PRESENTATIONTEXT; // PTD
        }
        // Image
        else if ((field->type[1] == 0x87 && field->type[2] == 0x8A) || // IOCA
                 (field->type[1] == 0xA7 && field->type[2] == 0xAB)) { // IRD
            field->obj_type = OBJ_IMAGE;                 
        }
        // Graphics
        else if (field->type[1] == 0xAF && field->type[2] == 0xA8) {
            field->obj_type = OBJ_GRAPHICS; // GAD
        }
        // Bar code
        else if (field->type[1] == 0xEB && field->type[2] == 0xBB) {
            field->obj_type = OBJ_BARCODE; // BDD
        }
    }
    
    // Font resources
    else if (field->type[0] == 0xD3 &&
             ((field->type[1] == 0x8A && field->type[2] == 0x8B) || // FND
              (field->type[1] == 0x8C && field->type[2] == 0x8A))) { // FNC
        field->obj_type = OBJ_FONT;
        field->component = COMPONENT_RESOURCE;
    }
    
    // Form definitions
    else if (field->type[0] == 0xD3 && field->type[1] == 0xA6 && field->type[2] == 0xAF) {
        field->obj_type = OBJ_FORMDEF; // FDG
        field->component = COMPONENT_RESOURCE;
    }
    
    // Resource naming fields
    if (field->type[0] == 0xD3 && field->type[1] == 0xA6 && field->type[2] == 0x89) { // MCF
        field->component = COMPONENT_RESOURCE;
        // Extract resource name from data if available
        if (field->data && field->length > 9) {
            // Names in AFP are typically 8 bytes max, in EBCDIC
            memcpy(field->name, field->data + 2, 8); 
            field->name[8] = '\0';
        }
    }
}

// Function to convert EBCDIC to ASCII (simplified)
char ebcdic_to_ascii(unsigned char ebcdic) {
    // Simple conversion for alphanumeric characters only
    if (ebcdic >= 0xC1 && ebcdic <= 0xC9) return 'A' + (ebcdic - 0xC1);
    if (ebcdic >= 0xD1 && ebcdic <= 0xD9) return 'J' + (ebcdic - 0xD1);
    if (ebcdic >= 0xE2 && ebcdic <= 0xE9) return 'S' + (ebcdic - 0xE2);
    if (ebcdic >= 0xF0 && ebcdic <= 0xF9) return '0' + (ebcdic - 0xF0);
    return '.';
}

// Function to print EBCDIC string in readable form
void print_ebcdic_string(const unsigned char *data, size_t length) {
    printf("EBCDIC: ");
    for (size_t i = 0; i < length; i++) {
        printf("%c", ebcdic_to_ascii(data[i]));
    }
    printf("\n");
}

void print_hex(const unsigned char *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0 && i < length - 1) {
            printf("\n         ");
        }
    }
    printf("\n");
}

const char* get_component_name(AFPComponent component) {
    switch(component) {
        case COMPONENT_DOCUMENT: return "Document";
        case COMPONENT_PAGE_GROUP: return "Page Group";
        case COMPONENT_PAGE: return "Page";
        case COMPONENT_OBJECT: return "Object";
        case COMPONENT_RESOURCE_GROUP: return "Resource Group";
        case COMPONENT_OVERLAY: return "Overlay";
        case COMPONENT_RESOURCE: return "Resource";
        default: return "Unknown";
    }
}

const char* get_object_type_name(AFPObjectType type) {
    switch(type) {
        case OBJ_PRESENTATIONTEXT: return "Presentation Text";
        case OBJ_IMAGE: return "Image";
        case OBJ_GRAPHICS: return "Graphics";
        case OBJ_BARCODE: return "Barcode";
        case OBJ_FONT: return "Font";
        case OBJ_PAGSEG: return "Page Segment";
        case OBJ_FORMDEF: return "Form Definition";
        case OBJ_RESLIB: return "Resource Library";
        default: return "Unknown";
    }
}

void print_ebcdic_type(const unsigned char *type) {
    printf("EBCDIC Type: %02X%02X%02X (", type[0], type[1], type[2]);
    
    // Common MO:DCA structured field types
    if (type[0] == 0xD3) {
        if (type[1] == 0xA8) {
            if (type[2] == 0xA8) printf("BDT - Begin Document");
            else if (type[2] == 0x92) printf("BOC - Begin Object Container");
            else if (type[2] == 0xCC) printf("BMM - Begin Medium Map");
            else if (type[2] == 0xD9) printf("BSG - Begin Resource Environment Group");
            else if (type[2] == 0xAF) printf("BPG - Begin Page");
            else if (type[2] == 0xAD) printf("BNG - Begin Named Page Group");
            else if (type[2] == 0xC9) printf("BAG - Begin Active Environment Group");
            else if (type[2] == 0xEB) printf("BBC - Begin Bar Code Object");
            else if (type[2] == 0x77) printf("BCA - Begin Color Attribute Table");
            else if (type[2] == 0x8A) printf("BCF - Begin Coded Font (BCF)");
            else if (type[2] == 0x87) printf("BCP - Begin Code Page (BCP)");
            else if (type[2] == 0xC4) printf("BDG - Begin Document Environment Group");
            else if (type[2] == 0xA7) printf("BDI - Begin Document Index");
            else if (type[2] == 0xC5) printf("BFG - Begin Form Environment Group (O)");
            else if (type[2] == 0xCD) printf("BFM - Begin Form Map");
            else if (type[2] == 0x89) printf("BFN - Begin Font (BFN)");
            else if (type[2] == 0xBB) printf("BGR - Begin Graphics Object");
            else if (type[2] == 0x7B) printf("BII - Begin IM Image (C)");
            else if (type[2] == 0xFB) printf("BIM - Begin Image Object");
            else if (type[2] == 0xDF) printf("BMO - Begin Overlay");
            else if (type[2] == 0xC7) printf("BOG - Begin Object Environment Group");
            else if (type[2] == 0x5F) printf("BPS - Begin Page Segment");
            else if (type[2] == 0x9B) printf("BPT - Begin Presentation Text Object");
            else if (type[2] == 0xC6) printf("BRG - Begin Resource Group");
            else if (type[2] == 0xCE) printf("BRS - Begin Resource");
            else printf("Unknown");
        } 
        else if (type[1] == 0xA9) {
            if (type[2] == 0xA8) printf("EDT - End Document");
            else if (type[2] == 0x92) printf("EOC - End Object Container");
            else if (type[2] == 0xCC) printf("EMM - End Medium Map");
            else if (type[2] == 0xD9) printf("BSG - End Resource Environment Group");
            else if (type[2] == 0xAF) printf("EPG - End Page");
            else if (type[2] == 0xAD) printf("BNG - End Named Page Group");
            else if (type[2] == 0xC9) printf("EAG - End Active Environment Group");
            else if (type[2] == 0xEB) printf("EBC - End Bar Code Object");
            else if (type[2] == 0x77) printf("ECA - End Color Attribute Table");
            else if (type[2] == 0x8A) printf("ECF - End Coded Font (BCF)");
            else if (type[2] == 0x87) printf("ECP - End Code Page (BCP)");
            else if (type[2] == 0xC4) printf("EDG - End Document Environment Group");
            else if (type[2] == 0xA7) printf("EDI - End Document Index");
            else if (type[2] == 0xC5) printf("EFG - End Form Environment Group (O)");
            else if (type[2] == 0xCD) printf("EFM - End Form Map");
            else if (type[2] == 0x89) printf("EFN - End Font (BFN)");
            else if (type[2] == 0xBB) printf("EGR - End Graphics Object");
            else if (type[2] == 0x7B) printf("EII - End IM Image (C)");
            else if (type[2] == 0xFB) printf("EIM - End Image Object");
            else if (type[2] == 0xDF) printf("EMO - End Overlay");
            else if (type[2] == 0xC7) printf("EOG - End Object Environment Group");
            else if (type[2] == 0x5F) printf("EPS - End Page Segment");
            else if (type[2] == 0x9B) printf("EPT - End Presentation Text Object");
            else if (type[2] == 0xC6) printf("ERG - End Resource Group");
            else if (type[2] == 0xCE) printf("ERS - End Resource");
            else printf("Unknown");
        } 
        else if (type[1] == 0x8C) {
            if (type[2] == 0x8A) printf("CFI - Coded Font Index (CFI)");
            else if (type[2] == 0x87) printf("CPI - Code Page Index (CPI)");
            else if (type[2] == 0x89) printf("FNI - Font Index (FNI)");
            else printf("Unknown");
        } 
        else if (type[1] == 0xA0) {
            if (type[2] == 0x88) printf("MFC - Medium Finishing Control");
            else if (type[2] == 0x90) printf("TLE - Tag Logical Element");
            else printf("Unknown");
        } 
        else if (type[1] == 0xA2) {
            if (type[2] == 0x89) printf("FNM - Font Patterns Map (FNM)");
            else if (type[2] == 0x88) printf("MCC - Medium Copy Count");
            else printf("Unknown");
        }
        else if (type[1] == 0xA6) {
            if (type[2] == 0x92) printf("CDD - Container Data Descriptor");
            else if (type[2] == 0x87) printf("CPD - Code Page Descriptor (CPD)");
            else if (type[2] == 0xC5) printf("FGD - Form Environment Group Descriptor (O)");
            else if (type[2] == 0x89) printf("FND - Font Descriptor (FND)");
            else if (type[2] == 0xBB) printf("GDD - Graphics Data Descriptor");
            else if (type[2] == 0xFB) printf("IDD - Image Data Descriptor");
            else if (type[2] == 0x7B) printf("IID - Image Input Descriptor (C)");
            else if (type[2] == 0x88) printf("MDD - Medium Descriptor");
            else if (type[2] == 0x6B) printf("OBD - Object Area Descriptor");
            else if (type[2] == 0xAF) printf("PGD - Page Descriptor");
            else if (type[2] == 0x9B) printf("PTD_1 - Presentation Text Descriptor Format-1 (C)");
            else if (type[2] == 0xEB) printf("BDD - Bar Code Data Descriptor");
            else printf("Unknown");
        }
        else if (type[1] == 0xA7) {
            if (type[2] == 0x8A) printf("CFC - Coded Font Control (CFC)");
            else if (type[2] == 0x87) printf("CPC - Code Page Control (CPC)");
            else if (type[2] == 0x9B) printf("CTC - Composed Text Control (O)");
            else if (type[2] == 0x89) printf("FNC - Font Control (FNC)");
            else if (type[2] == 0x7B) printf("IOC - IM Image Output Control (C)");
            else if (type[2] == 0x88) printf("MMC - Medium Modification Control");
            else if (type[2] == 0xA8) printf("PEC - Presentation Environment Control");
            else if (type[2] == 0xAF) printf("PMC - Page Modification Control");
            else if (type[2] == 0xAB) printf("IRD - Image Raster Data");
            else printf("Unknown");
        } 
        else if (type[1] == 0xAB) {
            if (type[2] == 0x89) printf("FNN - Font Name Map (FNN)");
            else if (type[2] == 0xCC) printf("IMM - Invoke Medium Map");
            else if (type[2] == 0xEB) printf("MBC - Map Bar Code Object");
            else if (type[2] == 0x77) printf("MCA - Map Color Attribute Table");
            else if (type[2] == 0x92) printf("MCD - Map Container Data");
            else if (type[2] == 0x8A) printf("MCF - Map Coded Font");
            else if (type[2] == 0xC3) printf("MDR - Map Data Resource");
            else if (type[2] == 0xBB) printf("MGO - Map Graphics Object");
            else if (type[2] == 0xFB) printf("MIO - Map Image Object");
            else if (type[2] == 0x88) printf("MMT - Map Media Type");
            else if (type[2] == 0xAF) printf("MPG - Map Page");
            else if (type[2] == 0xD8) printf("MPO - Map Page Overlay");
            else if (type[2] == 0xEA) printf("MSU - Map Suppression");
            else printf("Unknown");
        } 
        else if (type[1] == 0xAC) {
            if (type[2] == 0x89) printf("FNP - Font Position (FNP)");
            else if (type[2] == 0x7B) printf("IPC - IM Image Cell Position (C)");
            else if (type[2] == 0x6B) printf("OBP - Object Area Position");
            else if (type[2] == 0xAF) printf("PGP_1 - Page Position Format-1 (C)");
            else printf("Unknown");
        } 
        else if (type[1] == 0xAD && type[2] == 0xC3) {
            printf("PPO - Preprocess Presentation Object");
        }
        else if (type[1] == 0xAE && type[2] == 0x89) {
            printf("FNO - Font Orientation (FNO)");
        }
        else if (type[1] == 0xAF) {
            if (type[2] == 0xC3) printf("IOB - Include Object");
            else if (type[2] == 0xAF) printf("IPG - Include Page");
            else if (type[2] == 0xD8) printf("IPO - Include Page Overlay");
            else if (type[2] == 0x5F) printf("IPS - Include Page Segment");
            else if (type[2] == 0xA8) printf("GAD - Graphics Data");
            else printf("Unknown");
        }
        else if (type[1] == 0xB0 && type[2] == 0x77) {
            printf("CAT - Color Attribute Table");
        }
        else if (type[1] == 0xB1) {
            if (type[2] == 0x8A) printf("MCF_1 - Map Coded Font Format-1 (C)");
            else if (type[2] == 0xDF) printf("MMO - Map Medium Overlay");
            else if (type[2] == 0x5F) printf("MPS - Map Page Segment");
            else if (type[2] == 0xAF) printf("PGP - Page Position");
            else if (type[2] == 0x9B) printf("PTD - Presentation Text Data Descriptor");
            else printf("Unknown");
        }
        else if (type[1] == 0xB2) {
            if (type[2] == 0xA7) printf("IEL - Index Element");
            else if (type[2] == 0x88) printf("PFC - Presentation Fidelity Control");
            else if (type[2] == 0x90) printf("LLE - Link Logical Element");
            else if (type[2] == 0xAF) printf("PGP - Page Position");
            else if (type[2] == 0x9B) printf("PTD - Presentation Text Data Descriptor");
            else printf("Unknown");
        }
        else if (type[1] == 0xB4 && type[2] == 0x90) {
            printf("LLE - Link Logical Element");
        }
        else if (type[1] == 0xEE) {
            if (type[2] == 0x89) printf("FNG - Font Patterns (FNG)");
            else if (type[2] == 0xBB) printf("GAD - Graphics Data");
            else if (type[2] == 0xFB) printf("IPD - Image Picture Data");
            else if (type[2] == 0xEE) printf("NOP - No Operation");
            else if (type[2] == 0x92) printf("OCD - Object Container Data");
            else if (type[2] == 0x9B) printf("PTX - Presentation Text Data");
            else if (type[2] == 0xEB) printf("BDA - Bar Code Data");
            else printf("Unknown");
        }
        else {
            printf("Unknown");
        }
    } 
    else if (type[0] == 0xD9) {
        if (type[1] == 0xEE && type[2] == 0xD3) printf("NOP - No Operation");
        else printf("Unknown");
    } 
    else if (type[0] == 0x5A) {
        printf("Carriage Control");
    }
    else {
        printf("Unknown");
    }
    
    printf(")\n");
}

void print_structure_summary(ComponentStack *stack, int page_count, int object_count, int resource_count) {
    printf("\nAFP Structure Summary:\n");
    printf("---------------------\n");
    
    if (!stack_empty(stack)) {
        printf("Warning: Document structure is incomplete. Unclosed components:\n");
        
        while (!stack_empty(stack)) {
            AFPComponent comp = stack_pop(stack);
            printf("  - %s\n", get_component_name(comp));
        }
    } else {
        printf("Document structure is properly nested and complete.\n");
    }
    
    printf("\nContent Summary:\n");
    printf("  - Pages: %d\n", page_count);
    printf("  - Objects: %d\n", object_count);
    printf("  - Resources: %d\n", resource_count);
}

typedef struct {
    int documents;
    int page_groups;
    int pages;
    int overlays;
    int resource_groups;
    int presentation_text;
    int images;
    int graphics;
    int barcodes;
    int fonts;
    int form_defs;
    int page_segments;
} AFPStatistics;

void print_statistics(AFPStatistics *stats) {
    printf("\nAFP Content Statistics:\n");
    printf("----------------------\n");
    printf("Documents:         %d\n", stats->documents);
    printf("Page Groups:       %d\n", stats->page_groups);
    printf("Pages:             %d\n", stats->pages);
    printf("Overlays:          %d\n", stats->overlays);
    printf("Resource Groups:   %d\n", stats->resource_groups);
    printf("Presentation Text: %d\n", stats->presentation_text);
    printf("Images:            %d\n", stats->images);
    printf("Graphics:          %d\n", stats->graphics);
    printf("Barcodes:          %d\n", stats->barcodes);
    printf("Fonts:             %d\n", stats->fonts);
    printf("Form Definitions:  %d\n", stats->form_defs);
    printf("Page Segments:     %d\n", stats->page_segments);
}

void update_statistics(AFPStatistics *stats, StructuredField *field) {
    // Check Begin structured fields to count objects
    if (field->type[0] == 0xD3 && field->type[1] == 0xA8) {
        switch (field->type[2]) {
            case 0xC6: stats->documents++; break;       // BDT
            case 0xD6: stats->page_groups++; break;     // BNG
            case 0xCE: stats->pages++; break;           // BPG
            case 0xBB: stats->overlays++; break;        // BMO
            case 0xDF: stats->resource_groups++; break; // BRG
        }
    }
    
    // Count data objects
    if (field->obj_type == OBJ_PRESENTATIONTEXT) stats->presentation_text++;
    else if (field->obj_type == OBJ_IMAGE) stats->images++;
    else if (field->obj_type == OBJ_GRAPHICS) stats->graphics++;
    else if (field->obj_type == OBJ_BARCODE) stats->barcodes++;
    else if (field->obj_type == OBJ_FONT) stats->fonts++;
    else if (field->obj_type == OBJ_FORMDEF) stats->form_defs++;
    else if (field->obj_type == OBJ_PAGSEG) stats->page_segments++;
}

void print_logo(){
    //https://patorjk.com/software/taag/#p=testall&f=Big&t=AfpValidator
    printf("%s\n","            __   __      __   _ _     _       _             ");
    printf("%s\n","     /\\    / _|  \\ \\    / /  | (_)   | |     | |            ");
    printf("%s\n","    /  \\  | |_ _ _\\ \\  / /_ _| |_  __| | __ _| |_ ___  _ __ ");
    printf("%s\n","   / /\\ \\ |  _| '_ \\ \\/ / _` | | |/ _` |/ _` | __/ _ \\| '__|");
    printf("%s\n","  / ____ \\| | | |_) \\  / (_| | | | (_| | (_| | || (_) | |   ");
    printf("%s\n"," /_/    \\_\\_| | .__/ \\/ \\__,_|_|_|\\__,_|\\__,_|\\__\\___/|_|   ");
    printf("%s\n","              | |                                           ");
    printf("%s\n","              |_|                      By Began BALAKRISHNAN");
}
bool validate_afp_file(const char *filename, bool verbose) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return false;
    }

    bool is_valid = true;
    int field_count = 0;
    int error_count = 0;
    long file_size;
    
    // Component tracking
    ComponentStack component_stack;
    stack_init(&component_stack);
    
    int page_count = 0;
    int object_count = 0;
    int resource_count = 0;
    
    // Statistics
    AFPStatistics stats = {0};
    
    // Get file size
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    print_logo();
    printf("\n\nAnalyzing AFP file: %s (Size: %ld bytes)\n\n", filename, file_size);
    
    unsigned char buffer[4];
    long position = 0;
    bool has_begin_document = false;
    bool has_end_document = false;
    
    while (position < file_size) {
        // Read introducer
        if (fread(buffer, 1, 1, file) != 1) {
            if (feof(file)) break;
            printf("Error: Failed to read introducer at position %ld\n", position);
            is_valid = false;
            break;
        }
        
        if (buffer[0] != SF_INTRODUCER) {
            printf("Error: Invalid structured field introducer (0x%02X) at position %ld\n", 
                   buffer[0], position);
            is_valid = false;
            
            // Try to recover by seeking to the next byte
            position++;
            fseek(file, position, SEEK_SET);
            error_count++;
            
            if (error_count > 10) {
                printf("Too many errors, stopping analysis\n");
                break;
            }
            continue;
        }
        
        // Read length (2 bytes)
        if (fread(buffer, 1, 2, file) != 2) {
            printf("Error: Failed to read length at position %ld\n", position + 1);
            is_valid = false;
            break;
        }
        
        uint16_t length = (buffer[0] << 8) | buffer[1];
        
        // Validate length
        if (length < 5) {
            printf("Error: Invalid length (%d) at position %ld - too short\n", length, position + 1);
            is_valid = false;
            position += 3;
            fseek(file, position, SEEK_SET);
            error_count++;
            continue;
        }
        
        if (position + length > file_size) {
            printf("Error: Invalid length (%d) at position %ld - exceeds file size\n", 
                   length, position + 1);
            is_valid = false;
            position += 3;
            fseek(file, position, SEEK_SET);
            error_count++;
            continue;
        }
        
        // Read type (3 bytes)
        unsigned char type[3];
        if (fread(type, 1, 3, file) != 3) {
            printf("Error: Failed to read type at position %ld\n", position + 3);
            is_valid = false;
            break;
        }
        
        // Read flag byte
        unsigned char flag = 0;
        if (fread(&flag, 1, 1, file) != 1) {
            printf("Error: Failed to read flag byte at position %ld\n", position + 6);
            is_valid = false;
            break;
        }
        
        // Allocate memory for data
        unsigned char *data = NULL;
        int data_length = length - 6; // Introducer(1) + length(2) + type(3) + flag(1) - 1
        
        if (data_length > 0) {
            data = malloc(data_length);
            if (!data) {
                printf("Error: Memory allocation failed\n");
                is_valid = false;
                break;
            }
            
            if (fread(data, 1, data_length, file) != data_length) {
                printf("Error: Failed to read data at position %ld\n", position + 7);
                free(data);
                is_valid = false;
                break;
            }
        }
        
        // Prepare structured field
        StructuredField field;
        field.length = length;
        memcpy(field.type, type, 3);
        field.flags = flag;
        field.data = data;
        memset(field.name, 0, sizeof(field.name));
        
        // Identify field type and component
        identify_field_type(&field);
        
        // Check for Begin Document and End Document
        if (type[0] == 0xD3) {
            if(type[1] == 0xA8) {
                if (type[2] == 0xA8) { // BDT
                    has_begin_document = true;
                    stack_push(&component_stack, COMPONENT_DOCUMENT);
                }
                else if (type[2] == 0xAD) { // BNG - Begin Named Page Group
                    stack_push(&component_stack, COMPONENT_PAGE_GROUP);
                }
                else if (type[2] == 0xAF) { // BPG - Begin Page
                    stack_push(&component_stack, COMPONENT_PAGE);
                    page_count++;
                }
                else if (type[2] == 0xC9) { // BAG - Begin Active Environment Group
                    stack_push(&component_stack, COMPONENT_OBJECT);
                    object_count++;
                }
                else if (type[2] == 0xC6) { // BRG - Begin Resource Group
                    stack_push(&component_stack, COMPONENT_RESOURCE_GROUP);
                }
                else if (type[2] == 0xDF) { // BMO - Begin Medium Overlay
                    stack_push(&component_stack, COMPONENT_OVERLAY);
                }
            } else if(type[1] == 0xA9) {
                if (type[2] == 0xA8) { // EDT
                    has_end_document = true;
                    AFPComponent popped = stack_pop(&component_stack);
                    if (popped != COMPONENT_DOCUMENT) {
                        printf("Error: Document structure mismatch at position %ld\n", position);
                        printf("       Expected to end %s but found End Document\n", 
                            get_component_name(popped));
                        is_valid = false;
                    }
                }
                else if (type[2] == 0xAD) { // ENG - End Named Page Group
                    AFPComponent popped = stack_pop(&component_stack);
                    if (popped != COMPONENT_PAGE_GROUP) {
                        printf("Error: Document structure mismatch at position %ld\n", position);
                        printf("       Expected to end %s but found End Page Group\n", 
                            get_component_name(popped));
                        is_valid = false;
                    }
                }
                else if (type[2] == 0xAF) { // EPG - End Page
                    AFPComponent popped = stack_pop(&component_stack);
                    if (popped != COMPONENT_PAGE) {
                        printf("Error: Document structure mismatch at position %ld\n", position);
                        printf("       Expected to end %s but found End Page\n", 
                            get_component_name(popped));
                        is_valid = false;
                    }
                }
                else if (type[2] == 0xC9) { // EAG - End Active Environment Group
                    AFPComponent popped = stack_pop(&component_stack);
                    if (popped != COMPONENT_OBJECT) {
                        printf("Error: Document structure mismatch at position %ld\n", position);
                        printf("       Expected to end %s but found End Active Environment Group\n", 
                            get_component_name(popped));
                        is_valid = false;
                    }
                }
                else if (type[2] == 0xC6) { // ERG - End Resource Group
                    AFPComponent popped = stack_pop(&component_stack);
                    if (popped != COMPONENT_RESOURCE_GROUP) {
                        printf("Error: Document structure mismatch at position %ld\n", position);
                        printf("       Expected to end %s but found End Resource Group\n", 
                            get_component_name(popped));
                        is_valid = false;
                    }
                }
                else if (type[2] == 0xDF) { // EMO - End Medium Overlay
                    AFPComponent popped = stack_pop(&component_stack);
                    if (popped != COMPONENT_OVERLAY) {
                        printf("Error: Document structure mismatch at position %ld\n", position);
                        printf("       Expected to end %s but found End Medium Overlay\n", 
                            get_component_name(popped));
                        is_valid = false;
                    }
                }
            }
        }
        
        // Count resource
        if (field.component == COMPONENT_RESOURCE) {
            resource_count++;
        }
        
        // Update statistics
        update_statistics(&stats, &field);
        
        // Print field information
        if (verbose) {
            printf("Field #%d at position %ld:\n", field_count + 1, position);
            printf("  Introducer: 0x5A\n");
            printf("  Length: %d\n", length);
            printf("  Flag: 0x%02X\n", flag);
            printf("  ");
            print_ebcdic_type(type);
            
            if (field.component != COMPONENT_UNKNOWN) {
                printf("  Component: %s\n", get_component_name(field.component));
            }
            
            if (field.obj_type != OBJ_UNKNOWN) {
                printf("  Object Type: %s\n", get_object_type_name(field.obj_type));
            }
            
            if (field.name[0] != 0) {
                printf("  Resource Name: ");
                print_ebcdic_string((unsigned char*)field.name, strlen(field.name));
            }
            
            printf("  Data: ");
            if (data_length > 0) {
                print_hex(data, data_length);
                // Additional interpretation for common field types
                if (field.type[0] == 0xD3 && field.type[1] == 0xA8 && field.type[2] == 0xC6) {
                    // Document name for BDT
                    if (data_length >= 8) {
                        printf("  Document Name: ");
                        print_ebcdic_string(data, 8);
                    }
                }
            } else {
                printf("(none)\n");
            }
            printf("\n");
        }
        
        // Free allocated memory
        if (data) {
            free(data);
        }
        
        field_count++;
        position += length;
    }
    
    fclose(file);
    
    // Summary
    printf("\nAFP File Analysis Summary:\n");
    printf("-------------------------\n");
    printf("Total structured fields: %d\n", field_count);
    printf("Errors detected: %d\n", error_count);
    printf("Begin Document found: %s\n", has_begin_document ? "Yes" : "No");
    printf("End Document found: %s\n", has_end_document ? "Yes" : "No");
    
    if (!has_begin_document) {
        printf("Warning: No Begin Document structured field found\n");
    }
    
    if (!has_end_document) {
        printf("Warning: No End Document structured field found\n");
    }
    
    // Print structure summary
    print_structure_summary(&component_stack, page_count, object_count, resource_count);
    
    // Print statistics
    print_statistics(&stats);
    
    printf("\nValidation result: %s\n", is_valid ? "VALID" : "INVALID");
    
    return is_valid;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("AFP File Validator\n");
        printf("------------------\n");
        printf("Usage: %s <afp_file> [-v]\n", argv[0]);
        printf("  -v: Verbose mode (print details of each structured field)\n");
        printf("\nThis program validates AFP/MO:DCA files according to the specification.\n");
        printf("It analyzes the document structure, identifies errors, and provides statistics.\n");
        return 1;
    }
    
    const char *filename = argv[1];
    bool verbose = false;
    
    if (argc > 2 && strcmp(argv[2], "-v") == 0) {
        verbose = true;
    }
    
    validate_afp_file(filename, verbose);
    
    return 0;
}