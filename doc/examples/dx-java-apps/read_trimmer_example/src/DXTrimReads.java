import java.io.*;
import java.util.*;
import org.apache.commons.io.IOUtils;
import com.fasterxml.jackson.core.*;
import com.fasterxml.jackson.databind.*;
import com.fasterxml.jackson.databind.node.*;
import com.dnanexus.*;

public class DXTrimReads {
    public static void main(String[] args) throws Exception {
        System.out.println("This is the DNAnexus Java Read Trimmer Example App");
        ObjectMapper mapper = new ObjectMapper();

        String JobInput = IOUtils.toString(new FileInputStream("job_input.json"));
        JsonNode JobInputJson = (JsonNode)(new MappingJsonFactory().createJsonParser(JobInput).readValueAsTree());

        String gtableId = JobInputJson.get("reads").get("$dnanexus_link").textValue();
        int trimLength = JobInputJson.get("trimLength").intValue();

        System.out.println("Trimming reads in "+gtableId);
        
        JsonNode tableDesc = DXAPI.gtableDescribe(gtableId, mapper.readTree("{}"));

        ObjectNode gtableNewInput = mapper.createObjectNode();
        gtableNewInput.put("initializeFrom", mapper.createObjectNode());
        // ((ObjectNode)(gtableNewInput.get("initializeFrom"))).put("project", tableDesc.get("project"));
        ((ObjectNode)(gtableNewInput.get("initializeFrom"))).put("project", System.getenv("DX_WORKSPACE_ID"));
        ((ObjectNode)(gtableNewInput.get("initializeFrom"))).put("id", tableDesc.get("id"));
        String outputGTableId = DXAPI.gtableNew(gtableNewInput).get("id").textValue();

        int sequenceColumnIndex = -1, qualColumnIndex = -1;
        for (int i=0; i<tableDesc.get("columns").size(); i++) {
            if (tableDesc.get("columns").get(i).get("name").asText().equals("sequence")) {
                sequenceColumnIndex = i+1; // element 0 is the row index
            } else if (tableDesc.get("columns").get(i).get("name").asText().equals("quality")) {
                qualColumnIndex = i+1;
            }
        }
        
        int step = 10000;
        for (int i=0; i<tableDesc.get("length").intValue(); i += step) {
            ObjectNode gtableGetInput = mapper.createObjectNode();
            gtableGetInput.put("starting", i);
            gtableGetInput.put("limit", step);
            ArrayNode outputRows = mapper.createArrayNode();
            for (JsonNode row : DXAPI.gtableGet(gtableId, gtableGetInput).get("data")) {
                ArrayNode arrayRow = (ArrayNode)row;
                String sequence = arrayRow.get(sequenceColumnIndex).textValue();
                String quality = arrayRow.get(qualColumnIndex).textValue();
                String sequenceSubstr = sequence.substring(0, sequence.length()-trimLength);
                String qualitySubstr = quality.substring(0, quality.length()-trimLength);
                arrayRow.set(sequenceColumnIndex, TextNode.valueOf(sequenceSubstr));
                arrayRow.set(qualColumnIndex, TextNode.valueOf(qualitySubstr));
                outputRows.add(row);
            }
            ObjectNode gtableAddRowsInput = mapper.createObjectNode();
            gtableAddRowsInput.put("part", i);
            gtableAddRowsInput.put("data", outputRows);
            DXAPI.gtableAddRows(outputGTableId, gtableAddRowsInput);
        }
        DXAPI.gtableClose(outputGTableId, mapper.readTree("{}"));

        ObjectNode JobOutput = mapper.createObjectNode();
        JobOutput.put("trimmedReads", mapper.createObjectNode());
        ((ObjectNode)(JobOutput.get("trimmedReads"))).put("$dnanexus_link", outputGTableId);
        mapper.writeValue(new File("job_output.json"), JobOutput);
        System.out.println("Trimming complete!");
    }
}