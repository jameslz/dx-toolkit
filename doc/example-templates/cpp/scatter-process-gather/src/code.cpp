/*
 * DX_APP_WIZARD_NAME DX_APP_WIZARD_VERSION
 * Generated by dx-app-wizard.
 *
 * Scatter-process-gather execution pattern: Your app will split its
 * input into multiple pieces, each of which will be processed in
 * parallel, after which they are gathered together in some final
 * output.
 *
 * This pattern is very similar to the "parallelized" template.  What
 * it does differently is that it formally breaks out the "scatter"
 * phase as a separate black-box entry point in the app.  (As a side
 * effect, this requires a "map" entry point to call "process" on each
 * of the results from the "scatter" phase.)
 *
 * Note that you can also replace any entry point in this execution
 * pattern with an API call to run a separate app or applet.
 *
 * The following is a Unicode art picture of the flow of execution.
 * Each box is an entry point, and vertical lines indicate that the
 * entry point connected at the top of the line calls the entry point
 * connected at the bottom of the line.  The letters represent the
 * different stages in which the input is transformed, e.g. the output
 * of the "scatter" entry point ("array:B") is given to the "map"
 * entry point as input.  The "map" entry point calls as many
 * "process" entry points as there are elements in its array input and
 * gathers the results in its array output.
 *
 *          ┌──────┐
 *       A->│ main │->D (output from "postprocess")
 *          └┬─┬─┬─┘
 *           │ │ │
 *          ┌┴──────┐
 *       A->│scatter│->array:B
 *          └───────┘
 *             │ │
 *            ┌┴──────────────┐
 *   array:B->│      map      │->array:C
 *            └─────────┬─┬─┬─┘
 *               │      │ . .
 *               │     ┌┴──────┐
 *               │  B->│process│->C
 *               │     └───────┘
 *            ┌──┴────────┐
 *   array:C->│postprocess│->D
 *            └───────────┘
 *
 * A = original app input, split up by "scatter" into pieces of type B
 * B = an input that will be provided to a "process" entry point
 * C = the output of a "process" entry point
 * D = app output aggregated from the outputs of the "process" entry points
 *
 * Parallelized execution pattern: Your app will generate multiple
 * jobs to perform some computation in parallel, followed by a final
 * "postprocess" stage that will perform any additional computations
 * as necessary.
 *
 * See http://wiki.dnanexus.com/Developer-Portal for documentation and
 * tutorials on how to modify this file.
 *
 * By default, this template uses the DNAnexus C++ JSON library and
 * the C++ bindings.
 */

#include <iostream>
#include <vector>
#include <stdint.h>

#include "dxjson/dxjson.h"
#include "dxcpp/dxcpp.h"

using namespace std;
using namespace dx;

/**
 * This is the "gather" phase which aggregates and performs any
 * additional computation after the "map" (and therefore after all the
 * "process") jobs are done.
 */
void postprocess() {
  JSON input;
  dxLoadInput(input);

  for (int i = 0; i < input["process_outputs"].size(); i++) {
    cout << input["process_outputs"][i].toString() << endl;
  }

  JSON output = JSON(JSON_HASH);
  output["final_output"] = "postprocess placeholder output";
  dxWriteOutput(output);
}

/**
 * This is the "process" phase which performs computation on some
 * chunk of the input data (represented here by the input named
 * **scattered_input**).
 */
void process() {
  JSON input;
  dxLoadInput(input);

  cout << "scattered_input: " << input["scattered_input"].toString() << endl;
  cout << "additional_input: " << input["additional_input"].toString() << endl;

  // Fill in code here to process the input and create output.

  // As always, you can choose not to return output if the
  // "postprocess" stage does not require any input, e.g. rows have
  // been added to a GTable that has been created in advance.  Just
  // make sure that the "postprocess" job does not run until all
  // "process" jobs have finished by making it wait for "map" to
  // finish using the depends_on argument (this is already done for
  // you in the invocation of the "postprocess" job in "main").

  JSON output = JSON(JSON_HASH);
  output["process_output"] = "process placeholder output";
  dxWriteOutput(output);
}

/**
 * This is a mapping function which essentially calls the "process"
 * phase for each chunk of input data created by the "scatter" phase.
 */
void map_entry_point() {
  JSON input;
  dxLoadInput(input);

  cout << "array_of_scattered_input: " << input["array_of_scattered_input"].toString() << endl;
  cout << "process_input: " << input["process_input"].toString() << endl;

  // The following calls "process" for each of the items in
  // *array_of_scattered_input*, using as input the item in the array,
  // as well as the rest of the fields in *process_input*.
  JSON process_input = input["process_input"];

  vector<DXJob> subjobs;
  for (int i = 0; i < input["array_of_scattered_input"].size(); i++) {
    process_input["scattered_input"] = input["array_of_scattered_input"][i];
    subjobs.push_back(DXJob::newDXJob(process_input, "process"));
  }

  // The following aggregates the output of all the "process" subjobs
  // as the output of this function.
  vector<JSON> process_jbors;
  for (int i = 0; i < subjobs.size(); i++) {
    process_jbors.push_back(subjobs[i].getOutputRef("process_output"));
  }

  JSON output = JSON(JSON_HASH);
  output["process_outputs"] = process_jbors;
  dxWriteOutput(output);
}

/**
 * This is the scatter phase which divides some input to the
 * application into chunks to be processed by each of the "process"
 * entry points.
 */
void scatter() {
  JSON input;
  dxLoadInput(input);

  cout << "input_to_scatter: " << input["input_to_scatter"].toString() << endl;

  vector<string> array_of_scattered_input;
  array_of_scattered_input.push_back("placeholder1");
  array_of_scattered_input.push_back("placeholder2");

  JSON output = JSON(JSON_HASH);
  output["array_of_scattered_input"] = array_of_scattered_input;
  dxWriteOutput(output);
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    if (strcmp(argv[1], "process") == 0) {
      process();
      return 0;
    } else if (strcmp(argv[1], "postprocess") == 0) {
      postprocess();
      return 0;
    } else if (strcmp(argv[1], "scatter") == 0) {
      scatter();
      return 0;
    } else if (strcmp(argv[1], "map") == 0) {
      map_entry_point();
      return 0;
    } else if (strcmp(argv[1], "main") != 0) {
      return 1;
    }
  }

  JSON input;
  dxLoadInput(input);

  // The variable *input* should now contain the input fields given to
  // the app(let), with keys equal to the input field names.
  //
  // For example, if an input field is of name "num" and class "int",
  // you can obtain the value via:
  //
  // int num = input["num"].get<int>();
  //
  // See http://wiki.dnanexus.com/dxjson for more details on how to
  // use the C++ JSON library.
DX_APP_WIZARD_INITIALIZE_INPUTDX_APP_WIZARD_DOWNLOAD_ANY_FILES
  // We first create the "scatter" job which will scatter some input
  // (replace with your own input as necessary).

  JSON scatter_input = JSON(JSON_HASH);
  scatter_input["input_to_scatter"] = "placeholder value";
  DXJob scatter_job = DXJob::newDXJob(scatter_input, "scatter");

  // We will want to call "process" on each output of "scatter", so we
  // call the "map" entry point to do so.  We can also provide here
  // additional input that we want each "process" entry point to
  // receive, e.g. a GTable ID to which the "process" function should
  // add rows of data.
  JSON map_input = JSON(JSON_HASH);
  map_input["array_of_scattered_input"] = scatter_job.getOutputRef("array_of_scattered_input");
  map_input["process_input"] = JSON(JSON_HASH);
  map_input["process_input"]["additional_input"] = "gtable ID, for example";
  DXJob map_job = DXJob::newDXJob(map_input, "map");

  // Finally, we want the "postprocess" job to run after "map" is done
  // calling "process" on each of its inputs.  Note that a job is
  // marked as "done" only after all of its child jobs are also marked
  // "done".
  JSON postprocess_input = JSON(JSON_HASH);
  postprocess_input["process_outputs"] = map_job.getOutputRef("process_outputs");
  postprocess_input["additional_input"] = "gtable ID, for example";
  vector<string> depend_on_map_job(1, map_job.getID());
  DXJob postprocess_job = DXJob::newDXJob(postprocess_input, "postprocess",
                                          "", depend_on_map_job);
DX_APP_WIZARD_UPLOAD_ANY_FILES
  // If you would like to include any of the output fields from the
  // postprocess_job as the output of your app, you should return it
  // here using a job-based object reference.
  //
  // output["app_output_field"] = postprocess_job.getOutputRef("final_output");
  //
  // Tip: you can include in your output at this point any open
  // objects (such as gtables) which are closed by another entry
  // point that finishes later.  The system will check to make sure
  // that the output object is closed and will attempt to clone it
  // out as output into the parent container only after all subjobs
  // have finished.

  JSON output = JSON(JSON_HASH);
DX_APP_WIZARD_OUTPUT
  dxWriteOutput(output);

  return 0;
}
