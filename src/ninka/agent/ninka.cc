/*
 SPDX-FileCopyrightText: © 2014-2015 Siemens AG

 SPDX-License-Identifier: GPL-2.0-only
*/

#include "ninka.hpp"

using namespace fo;

int main(int argc, char** argv)
{
  /* before parsing argv and argc make sure */
  /* to initialize the scheduler connection */

  DbManager dbManager(&argc, argv);
  NinkaDatabaseHandler databaseHandler(dbManager);

  State state = getState(dbManager);

  while (fo_scheduler_next() != NULL)
  {
    int uploadId = atoi(fo_scheduler_current());

    if (uploadId == 0) continue;

    int arsId = writeARS(state, 0, uploadId, 0, dbManager);

    if (arsId <= 0)
      bail(5);

    if (!processUploadId(state, uploadId, databaseHandler))
      bail(2);

    fo_scheduler_heart(0);
    writeARS(state, arsId, uploadId, 1, dbManager);
  }
  fo_scheduler_heart(0);

  /* do not use bail, as it would prevent the destructors from running */
  fo_scheduler_disconnect(0);
  return 0;
}
